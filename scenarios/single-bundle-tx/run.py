#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    account_nonce,
    block_number,
    chain_head,
    chain_head_label,
    deployment_role,
    mine_bundle,
    print_step,
    public_transaction,
    public_transaction_payload,
    read_json,
    rpc,
    rpc_is_ready,
    submit_public_transaction,
    wait_for_builder,
)

SELF_TRANSFER_GAS = 21_000


def main() -> int:
    print_step("Checking existing local services")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]
    chain_id = int(deployment["chainId"])
    victim_role = deployment_role(deployment, "victim")
    victim = victim_role["address"]

    wait_for_builder()
    require_builder_controlled_mining(rpc_url)

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Victim address:  {victim}")

    print_step("Submitting one 0 ETH self-transfer to public mempool")
    head_before = chain_head()
    before_block_number = block_number(head_before)
    tx_payload = public_transaction_payload(
        chain_id=chain_id,
        nonce=account_nonce(rpc_url, victim),
        sender=victim,
        to=victim,
        calldata="0x",
        gas=SELF_TRANSFER_GAS,
    )
    mempool_record = submit_public_transaction(tx_payload)
    mempool_tx_id = mempool_record["mempoolTxId"]

    print(f"Head before:      {chain_head_label(head_before)}")
    print(f"Mempool tx id:    {mempool_tx_id}")
    print(f"Mempool seq num:  {mempool_record['seqNum']}")

    print_step("Mining one bundle with exactly that transaction")
    bundle_result = mine_bundle([{"mempoolTxId": mempool_tx_id}])
    tx_result = bundle_result["transactions"][0]
    head_after = chain_head()
    after_block_number = block_number(head_after)
    mined_block = block_by_number(rpc_url, after_block_number, include_transactions=True)
    final_record = public_transaction(mempool_tx_id)

    print(f"Head after:       {chain_head_label(head_after)}")
    print(f"Bundle status:    {bundle_result['status']}")
    print(f"Tx status:        {tx_result['status']}")
    print(f"Chain tx hash:    {tx_result['chainTxHash']}")
    print(f"Block tx count:   {len(mined_block['transactions'])}")

    assert_single_block_single_tx(
        before_block_number=before_block_number,
        after_block_number=after_block_number,
        bundle_result=bundle_result,
        tx_result=tx_result,
        final_record=final_record,
        mined_block=mined_block,
    )

    print_step("Scenario complete")
    print("One public self-transfer was included in one bundle and produced exactly one block transaction.")
    return 0


def require_running_deployment() -> dict[str, Any]:
    if not DEPLOYMENT_FILE.exists():
        raise ScenarioError(
            f"deployment file does not exist: {DEPLOYMENT_FILE}; run blockchain/bin/deploy-local.zsh first"
        )

    deployment = read_json(DEPLOYMENT_FILE)
    rpc_url = deployment.get("rpcUrl")
    if not isinstance(rpc_url, str) or rpc_url == "":
        raise ScenarioError("deployment does not contain rpcUrl")

    if not rpc_is_ready(rpc_url):
        raise ScenarioError(f"local chain RPC is not reachable at {rpc_url}")

    return deployment


def require_builder_controlled_mining(rpc_url: str) -> None:
    automine = rpc(rpc_url, "anvil_getAutomine", [])
    if automine is not False:
        raise ScenarioError(
            "local chain automine must be disabled so the block builder creates exactly one block"
        )


def block_by_number(rpc_url: str, number: int, include_transactions: bool) -> dict[str, Any]:
    block = rpc(rpc_url, "eth_getBlockByNumber", [hex(number), include_transactions])
    if not isinstance(block, dict):
        raise ScenarioError(f"block {number} not found")
    return block


def assert_single_block_single_tx(
    before_block_number: int,
    after_block_number: int,
    bundle_result: dict[str, Any],
    tx_result: dict[str, Any],
    final_record: dict[str, Any],
    mined_block: dict[str, Any],
) -> None:
    if bundle_result["status"] != "included":
        raise ScenarioError(f"expected included bundle, got {bundle_result['status']}")
    if tx_result["status"] != "included":
        raise ScenarioError(f"expected included tx, got {tx_result['status']}")
    if final_record["status"] != "included":
        raise ScenarioError(f"expected included mempool record, got {final_record['status']}")

    if after_block_number != before_block_number + 1:
        raise ScenarioError(
            f"expected exactly one new block, got before={before_block_number}, after={after_block_number}"
        )

    transactions = mined_block.get("transactions")
    if not isinstance(transactions, list):
        raise ScenarioError("mined block does not contain transactions list")
    if len(transactions) != 1:
        raise ScenarioError(f"expected exactly one transaction in mined block, got {len(transactions)}")

    block_hash = mined_block.get("hash")
    receipt = tx_result.get("receipt")
    if not isinstance(receipt, dict):
        raise ScenarioError("tx result does not contain receipt")
    if receipt.get("blockNumber") != hex(after_block_number):
        raise ScenarioError(
            f"expected receipt blockNumber {hex(after_block_number)}, got {receipt.get('blockNumber')}"
        )
    if receipt.get("blockHash") != block_hash:
        raise ScenarioError("receipt blockHash does not match mined block hash")
    if receipt.get("transactionIndex") != "0x0":
        raise ScenarioError(f"expected transactionIndex 0x0, got {receipt.get('transactionIndex')}")

    block_tx = transactions[0]
    block_tx_hash = block_tx.get("hash") if isinstance(block_tx, dict) else block_tx
    if block_tx_hash != tx_result["chainTxHash"]:
        raise ScenarioError(
            f"expected mined block tx {tx_result['chainTxHash']}, got {block_tx_hash}"
        )


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
