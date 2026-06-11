#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scenario_support import (  # noqa: E402
    BUILDER_URL,
    DEPLOYMENT_FILE,
    ScenarioError,
    account_nonce,
    chain_nonce,
    deployment_actor_roles,
    print_step,
    require_running_deployment,
    wait_for_builder,
)


def main() -> int:
    print_step("Checking existing local services")
    deployment = require_running_deployment()
    rpc_url = deployment["rpcUrl"]

    wait_for_builder()

    print(f"Deployment file: {DEPLOYMENT_FILE}")
    print(f"RPC URL:         {rpc_url}")
    print(f"Builder URL:     {BUILDER_URL}")

    actors = deployment_actor_roles(deployment)
    if not actors:
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


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ScenarioError as error:
        print(f"\nScenario failed: {error}", file=sys.stderr)
        raise SystemExit(1)
