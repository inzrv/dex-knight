#!/usr/bin/env python3
from __future__ import annotations

import sys
from collections import Counter
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    chain_head,
    chain_head_label,
    mine_bundle,
    pending_public_transactions,
    pending_records,
    print_step,
    require_builder_controlled_mining,
    require_running_deployment,
    wait_for_builder,
)


def main() -> int:
    print_step("Checking existing local services")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]

    wait_for_builder()
    require_builder_controlled_mining(rpc_url)

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Head before:     {chain_head_label(chain_head())}")

    print_step("Reading builder public mempool")
    before_snapshot = pending_public_transactions()
    before_records = pending_records(before_snapshot)
    print(f"Pending before:  {len(before_records)} tx(s)")

    if len(before_records) == 0:
        print_step("Scenario complete")
        print("Builder public mempool is already empty.")
        return 0

    bundle_items = build_bundle_items(before_records)
    print_step("Mining all pending mempool transactions in one bundle")
    bundle_result = mine_bundle(bundle_items)
    tx_results = bundle_result.get("transactions")
    if not isinstance(tx_results, list):
        raise ScenarioError("bundle response does not contain transactions list")

    after_snapshot = pending_public_transactions()
    after_records = pending_records(after_snapshot)

    print(f"Bundle status:   {bundle_result.get('status')}")
    print(f"Tx statuses:     {format_status_counts(tx_results)}")
    print(f"Pending after:   {len(after_records)} tx(s)")
    print(f"Head after:      {chain_head_label(chain_head())}")

    if len(after_records) != 0:
        remaining_ids = [str(record.get("mempoolTxId")) for record in after_records]
        raise ScenarioError(f"expected empty builder mempool, remaining tx ids: {', '.join(remaining_ids)}")

    print_step("Scenario complete")
    print("Builder public mempool was flushed through one private bundle.")
    return 0


def build_bundle_items(records: list[dict[str, Any]]) -> list[dict[str, str]]:
    items: list[dict[str, str]] = []
    for record in sorted(records, key=record_seq_num):
        mempool_tx_id = record.get("mempoolTxId")
        if not isinstance(mempool_tx_id, str) or mempool_tx_id == "":
            raise ScenarioError("pending record does not contain mempoolTxId")

        items.append({"mempoolTxId": mempool_tx_id})

    return items


def record_seq_num(record: dict[str, Any]) -> int:
    seq_num = record.get("seqNum")
    if not isinstance(seq_num, int):
        raise ScenarioError("pending record does not contain integer seqNum")
    return seq_num


def format_status_counts(tx_results: list[Any]) -> str:
    statuses = Counter()
    for result in tx_results:
        status = result.get("status") if isinstance(result, dict) else None
        statuses[str(status)] += 1

    return ", ".join(f"{status}={count}" for status, count in sorted(statuses.items()))


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
