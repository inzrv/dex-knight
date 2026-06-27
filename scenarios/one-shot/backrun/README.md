# Backrun Scenario

This scenario checks a simple backrun bundle over two sandbox pools.
It requires a clean local environment because it expects deterministic pool and backrun balances.
The scenario constructs the private bundle itself; it does not rely on Knight's
runtime loop.

## Steps

- starts a fresh local chain and block builder,
- requires `Pool1` and `Pool2` to be fresh and seeds each with `1,000 TokenA` and `1,000 TokenB`,
- mints `100 TokenA` to the victim and `42 TokenB` to `SandboxBackrun`,
- submits the victim `swapExactAForB` to the public mempool,
- sends a private bundle with the victim swap followed by `SandboxBackrun.executeBackrun`,
- verifies the victim swap succeeds and the backrun contract finishes with at least `4 TokenB` profit.

## Run

```shell
scenarios/one-shot/backrun/run.zsh
```
