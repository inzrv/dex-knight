#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    ScenarioError,
    TOKEN_DECIMALS,
    deployment_role,
    ensure_deployment,
    format_token_amount,
    mine_bundle,
    mint_token,
    pending_public_transactions,
    print_step,
    public_transaction,
    submit_public_transaction,
    start_block_builder,
    token_balance,
    token_transfer_payload,
)

TRANSFER_AMOUNT = TOKEN_DECIMALS
MINT_AMOUNT = 100 * TRANSFER_AMOUNT


def main() -> int:
    print_step("Preparing local chain")
    deployment = ensure_deployment()
    rpc_url = deployment["rpcUrl"]
    chain_id = int(deployment["chainId"])
    token_a = deployment["contracts"]["tokenA"]
    deployer_role = deployment_role(deployment, "deployer")
    victim_role = deployment_role(deployment, "victim")
    deployer = deployer_role["address"]
    recipient = victim_role["address"]
    deployer_key = deployer_role["privateKey"]

    print(f"Deployer address: {deployer}")
    print(f"Victim address:   {recipient}")

    print_step("Preparing local block builder")
    start_block_builder()

    print_step("Minting TokenA for the sender")
    mint_token(rpc_url, deployer_key, token_a, deployer, MINT_AMOUNT)

    sender_before = token_balance(rpc_url, token_a, deployer)
    recipient_before = token_balance(rpc_url, token_a, recipient)
    print(f"Sender TokenA before:    {format_token_amount(sender_before)}")
    print(f"Recipient TokenA before: {format_token_amount(recipient_before)}")

    print_step("Submitting transfer to the public mempool")
    tx_payload = token_transfer_payload(
        rpc_url,
        chain_id,
        deployer,
        token_a,
        recipient,
        TRANSFER_AMOUNT,
    )
    mempool_record = submit_public_transaction(tx_payload)
    mempool_tx_id = mempool_record["mempoolTxId"]
    print(f"Mempool transaction: {mempool_tx_id}")
    print(f"Initial status:      {mempool_record['status']}")

    pending = pending_public_transactions()
    print(f"Pending transactions: {len(pending['transactions'])}")

    print_step("Mining the transfer through the private bundle endpoint")
    bundle_result = mine_bundle([{"mempoolTxId": mempool_tx_id}])
    tx_result = bundle_result["transactions"][0]
    print(f"Bundle status:       {bundle_result['status']}")
    print(f"Chain transaction:   {tx_result['chainTxHash']}")
    print(f"Block number:        {int(tx_result['blockNumber'], 16)}")

    final_record = public_transaction(mempool_tx_id)
    sender_after = token_balance(rpc_url, token_a, deployer)
    recipient_after = token_balance(rpc_url, token_a, recipient)

    print_step("Checking final state")
    print(f"Final mempool status: {final_record['status']}")
    print(f"Sender TokenA after:  {format_token_amount(sender_after)}")
    print(f"Recipient after:      {format_token_amount(recipient_after)}")

    expected_recipient = recipient_before + TRANSFER_AMOUNT
    if bundle_result["status"] != "included":
        raise ScenarioError(f"expected included bundle, got {bundle_result['status']}")
    if final_record["status"] != "included":
        raise ScenarioError(f"expected included mempool record, got {final_record['status']}")
    if recipient_after != expected_recipient:
        raise ScenarioError(
            f"expected recipient balance {expected_recipient}, got {recipient_after}"
        )

    print_step("Scenario complete")
    print("Local chain and block builder are still running for inspection.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
