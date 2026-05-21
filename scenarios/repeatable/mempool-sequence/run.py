#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    ScenarioError,
    pending_public_transactions,
    print_step,
    restart_block_builder,
    submit_public_transaction,
)

ZERO_ADDRESS = "0x0000000000000000000000000000000000000000"


def main() -> int:
    print_step("Restarting local block builder")
    restart_block_builder()

    print_step("Checking empty public mempool snapshot")
    initial_snapshot = pending_public_transactions()
    assert_snapshot(initial_snapshot, expected_snapshot_seq=0, expected_seq_nums=[])
    print_snapshot("Initial snapshot", initial_snapshot)

    print_step("Submitting first public transaction")
    first_record, first_snapshot = submit_and_check_synthetic_transaction(
        expected_seq_num=1
    )
    print_record("First transaction", first_record)
    print_snapshot("Snapshot after first tx", first_snapshot)

    print_step("Submitting second public transaction")
    second_record, second_snapshot = submit_and_check_synthetic_transaction(
        expected_seq_num=2
    )
    print_record("Second transaction", second_record)
    print_snapshot("Snapshot after second tx", second_snapshot)

    print_step("Scenario complete")
    print("Public mempool seqNum and snapshotSeq advance monotonically.")
    return 0


def submit_and_check_synthetic_transaction(
    expected_seq_num: int,
) -> tuple[dict[str, Any], dict[str, Any]]:
    record = submit_public_transaction(synthetic_transaction_payload())
    assert_record(record, expected_seq_num=expected_seq_num)

    snapshot = pending_public_transactions()
    assert_snapshot(
        snapshot,
        expected_snapshot_seq=expected_seq_num,
        expected_seq_nums=list(range(1, expected_seq_num + 1)),
    )

    return record, snapshot


def synthetic_transaction_payload() -> dict[str, str]:
    # Intentionally synthetic: this scenario only checks builder mempool sequencing.
    return {
        "type": "0x0",
        "chainId": "0x0",
        "nonce": "0x0",
        "from": ZERO_ADDRESS,
        "to": ZERO_ADDRESS,
        "value": "0x0",
        "gas": "0x0",
        "gasPrice": "0x0",
        "input": "0x",
    }


def assert_record(record: dict[str, Any], expected_seq_num: int) -> None:
    seq_num = record.get("seqNum")
    if seq_num != expected_seq_num:
        raise ScenarioError(f"expected record seqNum {expected_seq_num}, got {seq_num}")

    status = record.get("status")
    if status != "pending":
        raise ScenarioError(f"expected pending record status, got {status}")


def assert_snapshot(
    snapshot: dict[str, Any],
    expected_snapshot_seq: int,
    expected_seq_nums: list[int],
) -> None:
    snapshot_seq = snapshot.get("snapshotSeq")
    if snapshot_seq != expected_snapshot_seq:
        raise ScenarioError(
            f"expected snapshotSeq {expected_snapshot_seq}, got {snapshot_seq}"
        )

    transactions = snapshot.get("transactions")
    if not isinstance(transactions, list):
        raise ScenarioError("expected snapshot transactions to be a list")

    if len(transactions) != len(expected_seq_nums):
        raise ScenarioError(
            f"expected {len(expected_seq_nums)} pending records, got {len(transactions)}"
        )

    seq_nums = sorted(record.get("seqNum") for record in transactions)
    if seq_nums != expected_seq_nums:
        raise ScenarioError(f"expected pending seq nums {expected_seq_nums}, got {seq_nums}")


def print_record(label: str, record: dict[str, Any]) -> None:
    print(f"{label} id:      {record['mempoolTxId']}")
    print(f"{label} seqNum:  {record['seqNum']}")
    print(f"{label} status:  {record['status']}")


def print_snapshot(label: str, snapshot: dict[str, Any]) -> None:
    print(f"{label} snapshotSeq: {snapshot['snapshotSeq']}")
    print(f"{label} tx count:    {len(snapshot['transactions'])}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
