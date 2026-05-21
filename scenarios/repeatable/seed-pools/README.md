# Seed Pools Scenario

## Lifecycle

- Group: `repeatable`.
- Can be run multiple times; each run adds another `100,000 TokenA` and `100,000 TokenB` to each pool.
- Starts or reuses the local chain when needed.

This scenario prepares both deployed AMM pools with equal `TokenA` / `TokenB`
liquidity.

It

- starts or reuses the local chain,
- mints enough `TokenA` and `TokenB` to the deployer,
- approves `Pool1` and `Pool2`, adds `100,000` of each token to each pool,
- verifies the reserve changes.

Run from the repository root:

```shell
scenarios/repeatable/seed-pools/run.zsh
```
