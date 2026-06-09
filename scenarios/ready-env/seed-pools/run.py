#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    TOKEN_DECIMALS,
    add_pool_liquidity,
    deployment_role,
    format_token_amount,
    pool_reserves,
    print_step,
    require_running_deployment,
    rpc,
)

POOL_TOKEN_AMOUNT = 1_000 * TOKEN_DECIMALS


def main() -> int:
    print_step("Checking existing local chain")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]
    contracts = deployment["contracts"]
    token_a = contracts["tokenA"]
    token_b = contracts["tokenB"]
    pool1 = contracts["pool1"]
    pool2 = contracts["pool2"]
    deployer_role = deployment_role(deployment, "deployer")
    deployer = deployer_role["address"]
    deployer_key = deployer_role["privateKey"]

    print(f"Deployment file:  {DEPLOYMENT_FILE}")
    print(f"RPC URL:          {rpc_url}")
    print(f"Deployer address: {deployer}")
    print(f"TokenA address:   {token_a}")
    print(f"TokenB address:   {token_b}")
    print(f"Pool1 address:    {pool1}")
    print(f"Pool2 address:    {pool2}")

    pools = [
        ("Pool1", pool1),
        ("Pool2", pool2),
    ]

    reserves_before = {
        label: pool_reserves(rpc_url, pool)
        for label, pool in pools
    }

    print_step("Adding pool seed liquidity")
    rpc(rpc_url, "evm_setAutomine", [True])
    try:
        for label, pool in pools:
            add_pool_liquidity(
                rpc_url,
                deployer_key,
                deployer,
                token_a,
                token_b,
                pool,
                POOL_TOKEN_AMOUNT,
                manage_automine=False,
            )
            print(f"{label} seeded with {format_token_amount(POOL_TOKEN_AMOUNT)} TokenA and TokenB")
    finally:
        rpc(rpc_url, "evm_setAutomine", [False])

    print_step("Checking reserves")
    for label, pool in pools:
        before_a, before_b = reserves_before[label]
        after_a, after_b = pool_reserves(rpc_url, pool)
        expected_a = before_a + POOL_TOKEN_AMOUNT
        expected_b = before_b + POOL_TOKEN_AMOUNT

        print(f"{label} reserveA before: {format_token_amount(before_a)}")
        print(f"{label} reserveB before: {format_token_amount(before_b)}")
        print(f"{label} reserveA after:  {format_token_amount(after_a)}")
        print(f"{label} reserveB after:  {format_token_amount(after_b)}")

        if after_a != expected_a:
            raise ScenarioError(f"{label} reserveA expected {expected_a}, got {after_a}")
        if after_b != expected_b:
            raise ScenarioError(f"{label} reserveB expected {expected_b}, got {after_b}")

    print_step("Scenario complete")
    print("Both pools received 1,000 TokenA and 1,000 TokenB.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
