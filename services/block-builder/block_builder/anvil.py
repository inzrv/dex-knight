from __future__ import annotations

import json
import time
from typing import Any, Optional
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from .transaction import Transaction


class AnvilClient:
    def __init__(self, rpcUrl: str) -> None:
        self._rpcUrl = rpcUrl
        self._requestId = 0

    def sendTransaction(self, transaction: Transaction) -> str:
        return self._rpc("eth_sendTransaction", [transaction.toRpcParams()])

    def mineBlock(self) -> None:
        self._rpc("evm_mine", [])

    def snapshot(self) -> str:
        return self._rpc("evm_snapshot", [])

    def revert(self, snapshotId: str) -> None:
        reverted = self._rpc("evm_revert", [snapshotId])
        if reverted is not True:
            raise RuntimeError(f"Anvil failed to revert snapshot {snapshotId}")

    def getLatestBlock(self) -> dict[str, Any]:
        return self._rpc("eth_getBlockByNumber", ["latest", False])

    def getLatestBlockNumber(self) -> int:
        block = self.getLatestBlock()
        blockNumber = block.get("number")
        if not isinstance(blockNumber, str):
            raise RuntimeError("Anvil latest block response does not contain a block number")

        try:
            return int(blockNumber, 16)
        except ValueError as error:
            raise RuntimeError(f"Anvil returned invalid latest block number: {blockNumber}") from error

    def call(self, callParams: dict[str, str], block: str) -> str:
        return self._rpc("eth_call", [callParams, block])

    def getReceipt(self, txHash: str) -> Optional[dict[str, Any]]:
        return self._rpc("eth_getTransactionReceipt", [txHash])

    def waitForReceipt(
        self,
        txHash: str,
        attempts: int = 20,
        delaySeconds: float = 0.1,
    ) -> Optional[dict[str, Any]]:
        for _ in range(attempts):
            receipt = self.getReceipt(txHash)
            if receipt is not None:
                return receipt

            time.sleep(delaySeconds)

        return None

    def _rpc(self, method: str, params: list[Any]) -> Any:
        self._requestId += 1
        body = json.dumps(
            {
                "jsonrpc": "2.0",
                "id": self._requestId,
                "method": method,
                "params": params,
            }
        ).encode("utf-8")

        request = Request(
            self._rpcUrl,
            data=body,
            headers={"Content-Type": "application/json"},
            method="POST",
        )

        try:
            with urlopen(request, timeout=10) as response:
                payload = json.loads(response.read().decode("utf-8"))
        except (HTTPError, URLError) as error:
            raise RuntimeError(f"Anvil RPC request failed: {error}") from error

        if "error" in payload:
            raise RuntimeError(f"Anvil RPC error: {payload['error']}")

        return payload.get("result")
