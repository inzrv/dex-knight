# Seed Pools Scenario

## Lifecycle

- Group: `ready-env`.
- Can be run multiple times; each run adds another `1,000 TokenA` and `1,000 TokenB` to each pool.
- Does not start, redeploy, or clean local services.
- Prepare a clean environment with `scenarios/ready-env/bin/start-clean.zsh` and clean it with `scenarios/ready-env/bin/cleanup.zsh`.

This scenario prepares both deployed AMM pools with equal `TokenA` / `TokenB`
liquidity.

It

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- mints enough `TokenA` and `TokenB` to the deployer,
- approves `Pool1` and `Pool2`, adds `1,000` of each token to each pool,
- verifies the reserve changes.

Run from the repository root:

```shell
scenarios/ready-env/seed-pools/run.zsh
```
