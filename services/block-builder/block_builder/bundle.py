from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Optional

from fastapi import HTTPException

from .anvil import AnvilClient
from .mempool import Mempool
from .transaction import Transaction


@dataclass(frozen=True)
class BundleItem:
    mempoolTxId: Optional[str]
    transaction: Transaction

    @classmethod
    def fromJson(cls, data: dict[str, Any], mempool: Mempool) -> "BundleItem":
        mempoolTxId = data.get("mempoolTxId")
        if mempoolTxId is not None:
            if not isinstance(mempoolTxId, str) or mempoolTxId == "":
                raise ValueError("'mempoolTxId' must be a non-empty string")

            record = mempool.getTransaction(mempoolTxId)
            if record is None:
                raise ValueError(f"mempool transaction '{mempoolTxId}' not found")

            if record.status != "pending":
                raise HTTPException(
                    status_code=409,
                    detail=f"mempool transaction '{mempoolTxId}' is '{record.status}', not 'pending'",
                )

            return cls(mempoolTxId=mempoolTxId, transaction=record.transaction)

        return cls(mempoolTxId=None, transaction=Transaction.fromJson(data))


@dataclass(frozen=True)
class Bundle:
    items: list[BundleItem]

    @classmethod
    def fromJson(cls, data: dict[str, Any], mempool: Mempool) -> "Bundle":
        rawItems = data.get("transactions")
        if not isinstance(rawItems, list) or len(rawItems) == 0:
            raise ValueError("'transactions' must be a non-empty list")

        return cls(
            items=[
                BundleItem.fromJson(rawItem, mempool)
                for rawItem in rawItems
            ]
        )


class BundleBuilder:
    def __init__(self, anvil: AnvilClient) -> None:
        self._anvil = anvil

    def simulateBundle(self, bundle: Bundle) -> dict[str, Any]:
        snapshotId = self._anvil.snapshot()
        try:
            result = self._executeBundle(bundle)
            result["simulated"] = True
            return result
        finally:
            self._anvil.revert(snapshotId)

    def mineBundle(self, bundle: Bundle, mempool: Mempool) -> dict[str, Any]:
        result = self._executeBundle(bundle)

        for item, txResult in zip(bundle.items, result["transactions"]):
            if item.mempoolTxId is not None:
                mempool.markMined(
                    mempoolTxId=item.mempoolTxId,
                    chainTxHash=txResult["chainTxHash"],
                    status=txResult["status"],
                    receipt=txResult["receipt"],
                )

        return result

    def _executeBundle(self, bundle: Bundle) -> dict[str, Any]:
        txHashes = [
            self._anvil.sendTransaction(item.transaction)
            for item in bundle.items
        ]

        self._anvil.mineBlock()

        receipts = [
            self._anvil.waitForReceipt(txHash)
            for txHash in txHashes
        ]

        return {
            "status": "included" if all(_statusFromReceipt(receipt) == "included" for receipt in receipts) else "failed",
            "transactions": [
                _txResult(item, txHash, receipt)
                for item, txHash, receipt in zip(bundle.items, txHashes, receipts)
            ],
        }


def _txResult(item: BundleItem, txHash: str, receipt: Optional[dict[str, Any]]) -> dict[str, Any]:
    if receipt is None:
        return {
            "mempoolTxId": item.mempoolTxId,
            "chainTxHash": txHash,
            "status": "missing",
            "receipt": None,
        }

    return {
        "mempoolTxId": item.mempoolTxId,
        "chainTxHash": txHash,
        "status": _statusFromReceipt(receipt),
        "blockNumber": receipt.get("blockNumber"),
        "transactionIndex": receipt.get("transactionIndex"),
        "receipt": receipt,
    }


def _statusFromReceipt(receipt: Optional[dict[str, Any]]) -> str:
    if receipt is None:
        return "missing"
    return "included" if receipt.get("status") == "0x1" else "reverted"
