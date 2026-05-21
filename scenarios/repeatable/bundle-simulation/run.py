#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    ScenarioError,
    TOKEN_DECIMALS,
    block_number,
    chain_head,
    chain_head_label,
    deployment_role,
    ensure_deployment,
    format_token_amount,
    mine_bundle,
    mint_token,
    print_step,
    simulate_bundle,
    start_block_builder,
    token_balance,
    token_transfer_payload,
)

MINT_AMOUNT = 3 * TOKEN_DECIMALS
REAL_TRANSFER_AMOUNT = 1 * TOKEN_DECIMALS
SIMULATED_TRANSFER_AMOUNT = 1 * TOKEN_DECIMALS


def main() -> int:
    print_step("Preparing local chain")
    deployment = ensure_deployment()
    rpc_url = deployment["rpcUrl"]
    chain_id = int(deployment["chainId"])
    token_a = deployment["contracts"]["tokenA"]
    deployer_role = deployment_role(deployment, "deployer")
    victim_role = deployment_role(deployment, "victim")
    deployer = deployer_role["address"]
    victim = victim_role["address"]
    deployer_key = deployer_role["privateKey"]

    print(f"Deployer address: {deployer}")
    print(f"Victim address:   {victim}")

    print_step("Preparing local block builder")
    start_block_builder()

    print_step("Minting TokenA for transfer checks")
    mint_token(rpc_url, deployer_key, token_a, deployer, MINT_AMOUNT)

    print_step("Mining a real transfer bundle")
    head_before_real = chain_head()
    real_payload = token_transfer_payload(
        rpc_url,
        chain_id,
        deployer,
        token_a,
        victim,
        REAL_TRANSFER_AMOUNT,
    )
    real_result = mine_bundle([real_payload])
    real_tx_result = real_result["transactions"][0]
    head_after_real = chain_head()

    print(f"Head before real tx: {chain_head_label(head_before_real)}")
    print(f"Head after real tx:  {chain_head_label(head_after_real)}")
    print(f"Real bundle status:  {real_result['status']}")
    print(f"Real tx status:      {real_tx_result['status']}")

    if real_result["status"] != "included":
        raise ScenarioError(f"expected included real bundle, got {real_result['status']}")
    if real_tx_result["status"] != "included":
        raise ScenarioError(f"expected included real tx, got {real_tx_result['status']}")
    if block_number(head_after_real) <= block_number(head_before_real):
        raise ScenarioError("expected real bundle to advance the chain head")

    print_step("Simulating another transfer bundle")
    deployer_before_sim = token_balance(rpc_url, token_a, deployer)
    victim_before_sim = token_balance(rpc_url, token_a, victim)
    head_before_sim = chain_head()
    simulation_payload = token_transfer_payload(
        rpc_url,
        chain_id,
        deployer,
        token_a,
        victim,
        SIMULATED_TRANSFER_AMOUNT,
    )
    simulation_result = simulate_bundle([simulation_payload])
    simulation_tx_result = simulation_result["transactions"][0]
    head_after_sim = chain_head()
    deployer_after_sim = token_balance(rpc_url, token_a, deployer)
    victim_after_sim = token_balance(rpc_url, token_a, victim)

    print(f"Head before simulation: {chain_head_label(head_before_sim)}")
    print(f"Head after simulation:  {chain_head_label(head_after_sim)}")
    print(f"Simulation status:      {simulation_result['status']}")
    print(f"Simulation tx status:   {simulation_tx_result['status']}")
    print(f"Deployer TokenA before sim:    {format_token_amount(deployer_before_sim)}")
    print(f"Deployer TokenA after sim:     {format_token_amount(deployer_after_sim)}")
    print(f"Victim TokenA before sim:      {format_token_amount(victim_before_sim)}")
    print(f"Victim TokenA after sim:       {format_token_amount(victim_after_sim)}")

    if simulation_result["status"] != "included":
        raise ScenarioError(f"expected included simulation, got {simulation_result['status']}")
    if simulation_result.get("simulated") is not True:
        raise ScenarioError("expected simulation response to include simulated=true")
    if simulation_tx_result["status"] != "included":
        raise ScenarioError(f"expected included simulated tx, got {simulation_tx_result['status']}")
    if head_after_sim != head_before_sim:
        raise ScenarioError("chain head changed after bundle simulation")
    if deployer_after_sim != deployer_before_sim:
        raise ScenarioError("deployer TokenA balance changed after bundle simulation")
    if victim_after_sim != victim_before_sim:
        raise ScenarioError("victim TokenA balance changed after bundle simulation")

    print_step("Scenario complete")
    print("Bundle simulation returned receipts without changing block head or balances.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
