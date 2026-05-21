#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    ScenarioError,
    TOKEN_DECIMALS,
    contract_transaction_payload,
    deployment_role,
    ensure_deployment,
    ensure_exact_pool_liquidity,
    format_token_amount,
    mine_bundle,
    mint_token,
    mint_and_approve,
    print_step,
    public_transaction,
    sandbox_amount_out,
    submit_public_transaction,
    start_block_builder,
    token_balance,
)

POOL_SEED_AMOUNT = 1_000 * TOKEN_DECIMALS
VICTIM_SWAP_AMOUNT_A = 100 * TOKEN_DECIMALS
BACKRUN_AMOUNT_B = 42 * TOKEN_DECIMALS
MIN_PROFIT_B = 4 * TOKEN_DECIMALS
MIN_OUTPUT_BPS = 9_900


def main() -> int:
    print_step("Preparing local chain")
    deployment = ensure_deployment()
    rpc_url = deployment["rpcUrl"]
    chain_id = int(deployment["chainId"])
    contracts = deployment["contracts"]
    token_a = contracts["tokenA"]
    token_b = contracts["tokenB"]
    pool1 = contracts["pool1"]
    pool2 = contracts["pool2"]
    backrun = contracts.get("backrun")
    if not isinstance(backrun, str) or backrun == "":
        raise ScenarioError("deployment does not contain SandboxBackrun; redeploy the blockchain module")

    deployer_role = deployment_role(deployment, "deployer")
    victim_role = deployment_role(deployment, "victim")
    bot_role = deployment_role(deployment, "bot")
    deployer = deployer_role["address"]
    victim = victim_role["address"]
    bot = bot_role["address"]

    print(f"Deployer address: {deployer}")
    print(f"Victim address:   {victim}")
    print(f"Bot address:      {bot}")
    print(f"Pool1 address:    {pool1}")
    print(f"Pool2 address:    {pool2}")
    print(f"Backrun address:  {backrun}")

    print_step("Preparing local block builder")
    start_block_builder()

    print_step("Checking deterministic pool state")
    pool1_reserves_before = ensure_exact_pool_liquidity(
        rpc_url,
        deployer_role["privateKey"],
        deployer,
        token_a,
        token_b,
        pool1,
        POOL_SEED_AMOUNT,
        "Pool1",
    )
    pool2_reserves_before = ensure_exact_pool_liquidity(
        rpc_url,
        deployer_role["privateKey"],
        deployer,
        token_a,
        token_b,
        pool2,
        POOL_SEED_AMOUNT,
        "Pool2",
    )

    backrun_a_before = token_balance(rpc_url, token_a, backrun)
    backrun_b_before = token_balance(rpc_url, token_b, backrun)
    if backrun_a_before != 0 or backrun_b_before != 0:
        raise ScenarioError(
            "backrun contract already holds tokens; clean and redeploy the local chain before rerunning"
        )

    print_step("Preparing victim TokenA and backrun TokenB")
    mint_and_approve(
        rpc_url,
        deployer_role["privateKey"],
        victim_role["privateKey"],
        victim,
        token_a,
        pool1,
        VICTIM_SWAP_AMOUNT_A,
    )
    mint_token(
        rpc_url,
        deployer_role["privateKey"],
        token_b,
        backrun,
        BACKRUN_AMOUNT_B,
    )

    victim_a_before = token_balance(rpc_url, token_a, victim)
    victim_b_before = token_balance(rpc_url, token_b, victim)
    backrun_b_after_mint = token_balance(rpc_url, token_b, backrun)
    if backrun_b_after_mint != BACKRUN_AMOUNT_B:
        raise ScenarioError(f"expected backrun contract to hold {BACKRUN_AMOUNT_B} TokenB, got {backrun_b_after_mint}")

    print_step("Calculating victim and backrun route")
    victim_amount_out_b = sandbox_amount_out(
        VICTIM_SWAP_AMOUNT_A,
        pool1_reserves_before[0],
        pool1_reserves_before[1],
    )
    victim_min_amount_out_b = victim_amount_out_b * MIN_OUTPUT_BPS // 10_000

    pool1_after_victim = (
        pool1_reserves_before[0] + VICTIM_SWAP_AMOUNT_A,
        pool1_reserves_before[1] - victim_amount_out_b,
    )
    backrun_amount_out_a = sandbox_amount_out(
        BACKRUN_AMOUNT_B,
        pool1_after_victim[1],
        pool1_after_victim[0],
    )
    backrun_amount_out_b = sandbox_amount_out(
        backrun_amount_out_a,
        pool2_reserves_before[0],
        pool2_reserves_before[1],
    )
    expected_profit_b = backrun_amount_out_b - BACKRUN_AMOUNT_B
    if expected_profit_b < MIN_PROFIT_B:
        raise ScenarioError(
            f"expected profit {expected_profit_b} is below the scenario minimum {MIN_PROFIT_B}"
        )

    backrun_min_amount_out_a = backrun_amount_out_a * MIN_OUTPUT_BPS // 10_000
    backrun_min_amount_out_b = backrun_amount_out_b * MIN_OUTPUT_BPS // 10_000

    print(f"Victim expected TokenB out:      {format_token_amount(victim_amount_out_b)}")
    print(f"Backrun TokenB input:            {format_token_amount(BACKRUN_AMOUNT_B)}")
    print(f"Backrun expected TokenA out:     {format_token_amount(backrun_amount_out_a)}")
    print(f"Backrun expected TokenB out:     {format_token_amount(backrun_amount_out_b)}")
    print(f"Backrun expected TokenB profit:  {format_token_amount(expected_profit_b)}")
    print(f"Backrun min TokenB profit:       {format_token_amount(MIN_PROFIT_B)}")

    print_step("Submitting victim swap to the public mempool")
    victim_record = submit_public_transaction(
        contract_transaction_payload(
            rpc_url,
            chain_id,
            victim,
            pool1,
            "swapExactAForB(uint256,uint256)",
            str(VICTIM_SWAP_AMOUNT_A),
            str(victim_min_amount_out_b),
        )
    )
    victim_mempool_tx_id = victim_record["mempoolTxId"]
    print(f"Victim mempool transaction: {victim_mempool_tx_id}")

    print_step("Submitting victim swap plus backrun bundle")
    bundle_result = mine_bundle(
        [
            {"mempoolTxId": victim_mempool_tx_id},
            contract_transaction_payload(
                rpc_url,
                chain_id,
                bot,
                backrun,
                "executeBackrun(address,address,uint256,uint256,uint256,uint256)",
                pool1,
                pool2,
                str(BACKRUN_AMOUNT_B),
                str(backrun_min_amount_out_a),
                str(backrun_min_amount_out_b),
                str(MIN_PROFIT_B),
                gas=700_000,
            ),
        ]
    )

    victim_tx_result = bundle_result["transactions"][0]
    backrun_tx_result = bundle_result["transactions"][1]
    print(f"Bundle status:         {bundle_result['status']}")
    print(f"Victim tx status:      {victim_tx_result['status']}")
    print(f"Backrun tx status:     {backrun_tx_result['status']}")
    print(f"Victim chain tx hash:  {victim_tx_result['chainTxHash']}")
    print(f"Backrun chain tx hash: {backrun_tx_result['chainTxHash']}")

    final_victim_record = public_transaction(victim_mempool_tx_id)
    victim_a_after = token_balance(rpc_url, token_a, victim)
    victim_b_after = token_balance(rpc_url, token_b, victim)
    backrun_a_after = token_balance(rpc_url, token_a, backrun)
    backrun_b_after = token_balance(rpc_url, token_b, backrun)
    realized_profit_b = backrun_b_after - backrun_b_after_mint

    print_step("Checking final balances")
    print(f"Victim spent TokenA:             {format_token_amount(victim_a_before - victim_a_after)}")
    print(f"Victim received TokenB:          {format_token_amount(victim_b_after - victim_b_before)}")
    print(f"Backrun TokenA final balance:    {format_token_amount(backrun_a_after)}")
    print(f"Backrun TokenB before backrun:   {format_token_amount(backrun_b_after_mint)}")
    print(f"Backrun TokenB after backrun:    {format_token_amount(backrun_b_after)}")
    print(f"Backrun realized TokenB profit:  {format_token_amount(realized_profit_b)}")

    if bundle_result["status"] != "included":
        raise ScenarioError(f"expected included bundle, got {bundle_result['status']}")
    if victim_tx_result["status"] != "included":
        raise ScenarioError(f"expected included victim swap, got {victim_tx_result['status']}")
    if backrun_tx_result["status"] != "included":
        raise ScenarioError(f"expected included backrun tx, got {backrun_tx_result['status']}")
    if final_victim_record["status"] != "included":
        raise ScenarioError(f"expected included victim mempool record, got {final_victim_record['status']}")
    if victim_a_before - victim_a_after != VICTIM_SWAP_AMOUNT_A:
        raise ScenarioError("victim did not spend the expected TokenA amount")
    if victim_b_after - victim_b_before < victim_min_amount_out_b:
        raise ScenarioError("victim received less TokenB than requested")
    if backrun_a_after != 0:
        raise ScenarioError(f"expected backrun contract TokenA balance to be zero, got {backrun_a_after}")
    if realized_profit_b < MIN_PROFIT_B:
        raise ScenarioError(
            f"expected backrun profit at least {MIN_PROFIT_B}, got {realized_profit_b}"
        )

    print_step("Scenario complete")
    print("Victim swap and backrun bundle were mined with positive TokenB profit.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
