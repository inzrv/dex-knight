#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    approve_token,
    deployment_role,
    print_step,
    require_running_deployment,
    rpc,
    token_allowance,
)

MAX_UINT256 = (1 << 256) - 1


def main() -> int:
    print_step("Checking existing local chain")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]
    contracts = deployment["contracts"]
    token_a = contracts["tokenA"]
    token_b = contracts["tokenB"]
    pool1 = contracts["pool1"]
    pool2 = contracts["pool2"]
    victim_role = deployment_role(deployment, "victim")
    victim = victim_role["address"]
    victim_key = victim_role["privateKey"]

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Victim address:  {victim}")
    print(f"TokenA address:  {token_a}")
    print(f"TokenB address:  {token_b}")
    print(f"Pool1 address:   {pool1}")
    print(f"Pool2 address:   {pool2}")

    approvals = [
        ("TokenA", token_a, "Pool1", pool1),
        ("TokenA", token_a, "Pool2", pool2),
        ("TokenB", token_b, "Pool1", pool1),
        ("TokenB", token_b, "Pool2", pool2),
    ]
    before = read_allowances(rpc_url, victim, approvals)

    print_step("Approving pools")
    rpc(rpc_url, "evm_setAutomine", [True])
    try:
        for token_label, token, pool_label, pool in approvals:
            approve_token(
                rpc_url,
                victim_key,
                token,
                pool,
                MAX_UINT256,
                manage_automine=False,
            )
            print(f"{token_label} approved for {pool_label}")
    finally:
        rpc(rpc_url, "evm_setAutomine", [False])

    after = read_allowances(rpc_url, victim, approvals)

    print_step("Checking allowances")
    for approval in approvals:
        token_label, _, pool_label, _ = approval
        key = approval_key(approval)
        print(f"{token_label} -> {pool_label} before: {before[key]}")
        print(f"{token_label} -> {pool_label} after:  {after[key]}")
        if after[key] != MAX_UINT256:
            raise ScenarioError(
                f"{token_label} allowance for {pool_label} expected uint256 max, got {after[key]}"
            )

    print_step("Scenario complete")
    print("Victim granted unlimited TokenA and TokenB approvals to Pool1 and Pool2.")
    return 0


def read_allowances(
    rpc_url: str,
    owner: str,
    approvals: list[tuple[str, str, str, str]],
) -> dict[str, int]:
    return {
        approval_key(approval): token_allowance(rpc_url, approval[1], owner, approval[3])
        for approval in approvals
    }


def approval_key(approval: tuple[str, str, str, str]) -> str:
    token_label, _, pool_label, _ = approval
    return f"{token_label}:{pool_label}"


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
