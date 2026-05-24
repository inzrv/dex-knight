#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    TOKEN_DECIMALS,
    account_nonce,
    block_number,
    chain_head,
    chain_head_label,
    contract_calldata,
    deployment_role,
    pending_public_transactions,
    pending_records,
    public_transaction_payload,
    print_step,
    require_builder_controlled_mining,
    require_running_deployment,
    submit_public_transaction,
    wait_for_builder,
)

SWAP_AMOUNT_A = 100 * TOKEN_DECIMALS
SWAP_MIN_AMOUNT_OUT_B = 0
SWAP_SELECTOR = "0x4cabca0a"


def main() -> int:
    print_step("Checking existing local services")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]
    chain_id = int(deployment["chainId"])
    contracts = deployment["contracts"]
    pool1 = contracts["pool1"]
    victim_role = deployment_role(deployment, "victim")
    victim = victim_role["address"]

    wait_for_builder()
    require_builder_controlled_mining(rpc_url)

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Victim address:  {victim}")
    print(f"Pool1 address:   {pool1}")

    before_head = chain_head()
    before_block = block_number(before_head)
    before_records = pending_records(pending_public_transactions())

    print_step("Submitting victim swap to builder public mempool")
    nonce = next_nonce_for_sender(rpc_url, victim, before_records)
    calldata = contract_calldata(
        "swapExactAForB(uint256,uint256)",
        str(SWAP_AMOUNT_A),
        str(SWAP_MIN_AMOUNT_OUT_B),
    )
    tx_payload = public_transaction_payload(
        chain_id=chain_id,
        nonce=nonce,
        sender=victim,
        to=pool1,
        calldata=calldata,
    )
    mempool_record = submit_public_transaction(tx_payload)
    mempool_tx_id = mempool_record["mempoolTxId"]

    after_head = chain_head()
    after_block = block_number(after_head)
    after_records = pending_records(pending_public_transactions())

    print(f"Head before:      {chain_head_label(before_head)}")
    print(f"Head after:       {chain_head_label(after_head)}")
    print(f"Mempool before:   {len(before_records)} pending tx(s)")
    print(f"Mempool after:    {len(after_records)} pending tx(s)")
    print(f"Victim nonce:     {nonce}")
    print(f"Mempool tx id:    {mempool_tx_id}")
    print(f"Mempool seq num:  {mempool_record['seqNum']}")

    assert_pending_swap(
        before_block=before_block,
        after_block=after_block,
        before_records=before_records,
        after_records=after_records,
        mempool_record=mempool_record,
        expected_to=pool1,
    )

    print_step("Scenario complete")
    print("Victim swap was submitted to the builder public mempool without mining a block.")
    return 0


def next_nonce_for_sender(rpc_url: str, sender: str, records: list[dict[str, Any]]) -> int:
    nonce = account_nonce(rpc_url, sender)
    for record in records:
        transaction = record.get("transaction")
        if not isinstance(transaction, dict):
            continue

        tx_from = transaction.get("from")
        tx_nonce = transaction.get("nonce")
        if isinstance(tx_from, str) and tx_from.lower() == sender.lower() and isinstance(tx_nonce, str):
            nonce = max(nonce, int(tx_nonce, 16) + 1)

    return nonce


def assert_pending_swap(
    before_block: int,
    after_block: int,
    before_records: list[dict[str, Any]],
    after_records: list[dict[str, Any]],
    mempool_record: dict[str, Any],
    expected_to: str,
) -> None:
    if after_block != before_block:
        raise ScenarioError(f"expected no new blocks, got before={before_block}, after={after_block}")

    if mempool_record.get("status") != "pending":
        raise ScenarioError(f"expected pending mempool record, got {mempool_record.get('status')}")

    if len(after_records) != len(before_records) + 1:
        raise ScenarioError(
            f"expected pending mempool size to increase by 1, got before={len(before_records)}, after={len(after_records)}"
        )

    mempool_tx_id = mempool_record.get("mempoolTxId")
    if not isinstance(mempool_tx_id, str):
        raise ScenarioError("builder response does not contain mempoolTxId")

    if not any(record.get("mempoolTxId") == mempool_tx_id for record in after_records):
        raise ScenarioError(f"submitted transaction {mempool_tx_id} not found in pending snapshot")

    transaction = mempool_record.get("transaction")
    if not isinstance(transaction, dict):
        raise ScenarioError("mempool record does not contain transaction object")

    tx_to = transaction.get("to")
    if not isinstance(tx_to, str) or tx_to.lower() != expected_to.lower():
        raise ScenarioError(f"expected transaction to {expected_to}, got {tx_to}")

    tx_input = transaction.get("input")
    if not isinstance(tx_input, str) or not tx_input.startswith(SWAP_SELECTOR):
        raise ScenarioError(f"expected swapExactAForB calldata selector {SWAP_SELECTOR}, got {tx_input}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
