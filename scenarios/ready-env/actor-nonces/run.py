#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    BUILDER_URL,
    DEPLOYMENT_FILE,
    ScenarioError,
    account_nonce,
    chain_nonce,
    deployment_role,
    print_step,
    require_running_deployment,
    wait_for_builder,
)

PREFERRED_ROLE_ORDER = ("deployer", "victim", "bot", "treasury")


def main() -> int:
    print_step("Checking existing local services")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]

    wait_for_builder()

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Builder URL:     {BUILDER_URL}")

    actors = actor_roles(deployment)
    if len(actors) == 0:
        raise ScenarioError("deployment does not contain actor roles")

    print_step("Reading actor nonces")
    for name, address in actors:
        builder_nonce = chain_nonce(address)
        rpc_nonce = account_nonce(rpc_url, address)
        print(f"{name:<10} {address}  nonce={builder_nonce}")

        if builder_nonce != rpc_nonce:
            raise ScenarioError(
                f"nonce mismatch for {name} {address}: builder={builder_nonce}, rpc={rpc_nonce}"
            )

    print_step("Scenario complete")
    print("Builder nonce endpoint returned the current RPC nonce for every deployment actor.")
    return 0


def actor_roles(deployment: dict[str, Any]) -> list[tuple[str, str]]:
    roles = deployment.get("roles")
    if isinstance(roles, dict):
        return sorted(
            (
                (name, role["address"])
                for name, role in roles.items()
                if isinstance(name, str)
                and isinstance(role, dict)
                and isinstance(role.get("address"), str)
                and role["address"] != ""
            ),
            key=lambda item: role_sort_key(item[0]),
        )

    actors: list[tuple[str, str]] = []
    for name in PREFERRED_ROLE_ORDER:
        try:
            role = deployment_role(deployment, name)
        except ScenarioError:
            continue

        address = role.get("address")
        if isinstance(address, str) and address != "":
            actors.append((name, address))

    return actors


def role_sort_key(name: str) -> tuple[int, str]:
    try:
        return (PREFERRED_ROLE_ORDER.index(name), name)
    except ValueError:
        return (len(PREFERRED_ROLE_ORDER), name)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
