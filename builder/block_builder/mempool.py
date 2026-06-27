from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from threading import Lock
from typing import Any, Optional
from uuid import uuid4

from .transaction import Transaction


@dataclass
class MempoolRecord:
    mempoolTxId: str
    seqNum: int
    transaction: Transaction
    status: str
    submittedAt: str
    chainTxHash: Optional[str] = None
    receipt: Optional[dict[str, Any]] = None
    updatedAt: Optional[str] = None

    def toJson(self) -> dict[str, Any]:
        return {
            "mempoolTxId": self.mempoolTxId,
            "seqNum": self.seqNum,
            "transaction": self.transaction.toJson(),
            "status": self.status,
            "submittedAt": self.submittedAt,
            "chainTxHash": self.chainTxHash,
            "receipt": self.receipt,
            "updatedAt": self.updatedAt,
        }


class Mempool:
    def __init__(self) -> None:
        self._transactions: dict[str, MempoolRecord] = {}
        self._lastSeqNum = 0
        self._lock = Lock()

    def addTransaction(self, transaction: Transaction) -> MempoolRecord:
        with self._lock:
            self._lastSeqNum += 1
            mempoolTxId = f"mp-{uuid4().hex}"

            record = MempoolRecord(
                mempoolTxId=mempoolTxId,
                seqNum=self._lastSeqNum,
                transaction=transaction,
                status="pending",
                submittedAt=_local_now(),
            )
            self._transactions[mempoolTxId] = record
            return record

    def getTransaction(self, mempoolTxId: str) -> Optional[MempoolRecord]:
        with self._lock:
            return self._transactions.get(mempoolTxId)

    def markMined(
        self,
        mempoolTxId: str,
        chainTxHash: str,
        status: str,
        receipt: Optional[dict[str, Any]],
    ) -> MempoolRecord:
        with self._lock:
            record = self._transactions.get(mempoolTxId)
            if record is None:
                raise ValueError(f"transaction '{mempoolTxId}' not found")

            record.status = status
            record.chainTxHash = chainTxHash
            record.receipt = receipt
            record.updatedAt = _local_now()
            return record

    def pendingSnapshot(self) -> dict[str, Any]:
        with self._lock:
            return {
                "snapshotSeq": self._lastSeqNum,
                "transactions": [
                    record.toJson()
                    for record in self._transactions.values()
                    if record.status == "pending"
                ],
            }


def _local_now() -> str:
    return datetime.now().astimezone().isoformat()
