# Seed Pools Scenario

This scenario prepares both deployed AMM pools with equal `TokenA` / `TokenB`
liquidity.
Each run adds another `1,000 TokenA` and `1,000 TokenB` to each pool.
Knight snapshots these reserves through the builder `POST /chain/call` endpoint
on new blocks.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- mints enough `TokenA` and `TokenB` to the deployer,
- approves `Pool1` and `Pool2`, adds `1,000` of each token to each pool,
- verifies the reserve changes.

## Run

```shell
scenarios/ready-env/seed-pools/run.zsh
```
