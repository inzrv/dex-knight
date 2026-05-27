#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    DEPLOYMENT_FILE,
    ScenarioError,
    TOKEN_DECIMALS,
    deployment_role,
    format_token_amount,
    mint_token,
    print_step,
    require_running_deployment,
    rpc,
    token_balance,
)

TOKEN_AMOUNT = 1_000 * TOKEN_DECIMALS


def main() -> int:
    print_step("Checking existing local chain")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]
    contracts = deployment["contracts"]
    token_a = contracts["tokenA"]
    token_b = contracts["tokenB"]
    deployer_role = deployment_role(deployment, "deployer")
    victim_role = deployment_role(deployment, "victim")
    deployer_key = deployer_role["privateKey"]
    victim = victim_role["address"]

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Victim address:  {victim}")
    print(f"TokenA address:  {token_a}")
    print(f"TokenB address:  {token_b}")

    before_a = token_balance(rpc_url, token_a, victim)
    before_b = token_balance(rpc_url, token_b, victim)

    print_step("Minting victim tokens")
    rpc(rpc_url, "evm_setAutomine", [True])
    try:
        mint_token(
            rpc_url,
            deployer_key,
            token_a,
            victim,
            TOKEN_AMOUNT,
            manage_automine=False,
        )
        mint_token(
            rpc_url,
            deployer_key,
            token_b,
            victim,
            TOKEN_AMOUNT,
            manage_automine=False,
        )
    finally:
        rpc(rpc_url, "evm_setAutomine", [False])

    after_a = token_balance(rpc_url, token_a, victim)
    after_b = token_balance(rpc_url, token_b, victim)

    print_step("Checking victim balances")
    print(f"TokenA before:   {format_token_amount(before_a)}")
    print(f"TokenA after:    {format_token_amount(after_a)}")
    print(f"TokenB before:   {format_token_amount(before_b)}")
    print(f"TokenB after:    {format_token_amount(after_b)}")

    assert_balance_delta("TokenA", before_a, after_a, TOKEN_AMOUNT)
    assert_balance_delta("TokenB", before_b, after_b, TOKEN_AMOUNT)

    print_step("Scenario complete")
    print(f"Victim received {format_token_amount(TOKEN_AMOUNT)} TokenA and TokenB.")
    return 0


def assert_balance_delta(label: str, before: int, after: int, expected_delta: int) -> None:
    actual_delta = after - before
    if actual_delta != expected_delta:
        raise ScenarioError(
            f"{label} balance delta expected {expected_delta}, got {actual_delta}"
        )


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
