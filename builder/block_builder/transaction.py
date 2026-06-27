from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Optional


@dataclass(frozen=True)
class Transaction:
    hash: Optional[str]
    type: Optional[str]
    chainId: Optional[str]
    nonce: str
    fromAddress: str
    toAddress: Optional[str]
    value: str
    gas: str
    gasPrice: Optional[str]
    maxFeePerGas: Optional[str]
    maxPriorityFeePerGas: Optional[str]
    input: str

    @classmethod
    def fromJson(cls, data: dict[str, Any]) -> "Transaction":
        return cls(
            hash=_optional_str(data, "hash"),
            type=_optional_str(data, "type"),
            chainId=_optional_str(data, "chainId"),
            nonce=_required_str(data, "nonce"),
            fromAddress=_required_str(data, "from"),
            toAddress=_optional_str(data, "to"),
            value=_optional_str(data, "value") or "0x0",
            gas=_required_str(data, "gas"),
            gasPrice=_optional_str(data, "gasPrice"),
            maxFeePerGas=_optional_str(data, "maxFeePerGas"),
            maxPriorityFeePerGas=_optional_str(data, "maxPriorityFeePerGas"),
            input=_required_str(data, "input"),
        )

    def toJson(self) -> dict[str, Any]:
        return {
            "hash": self.hash,
            "type": self.type,
            "chainId": self.chainId,
            "nonce": self.nonce,
            "from": self.fromAddress,
            "to": self.toAddress,
            "value": self.value,
            "gas": self.gas,
            "gasPrice": self.gasPrice,
            "maxFeePerGas": self.maxFeePerGas,
            "maxPriorityFeePerGas": self.maxPriorityFeePerGas,
            "input": self.input,
        }

    def toRpcParams(self) -> dict[str, Any]:
        params = {
            "from": self.fromAddress,
            "to": self.toAddress,
            "nonce": self.nonce,
            "value": self.value,
            "gas": self.gas,
            "data": self.input,
        }

        if self.gasPrice is not None:
            params["gasPrice"] = self.gasPrice

        if self.maxFeePerGas is not None:
            params["maxFeePerGas"] = self.maxFeePerGas

        if self.maxPriorityFeePerGas is not None:
            params["maxPriorityFeePerGas"] = self.maxPriorityFeePerGas

        return params


def _required_str(data: dict[str, Any], key: str) -> str:
    value = data.get(key)
    if not isinstance(value, str) or value == "":
        raise ValueError(f"'{key}' must be a non-empty string")
    return value


def _optional_str(data: dict[str, Any], key: str) -> Optional[str]:
    value = data.get(key)
    if value is None:
        return None
    if not isinstance(value, str):
        raise ValueError(f"'{key}' must be a string")
    return value
