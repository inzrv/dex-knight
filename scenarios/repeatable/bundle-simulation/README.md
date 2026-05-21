# Bundle Simulation Scenario

## Lifecycle

- Group: `repeatable`.
- Can be run multiple times; it checks deltas and verifies simulation does not mutate chain state.
- Starts or reuses the local chain and block builder when needed.

This scenario checks that bundle simulation produces execution results without
changing the real chain state.

It

- starts or reuses the local chain and block builder,
- mints `3 TokenA` to the deployer,
- mines one real `TokenA` transfer through `POST /private/bundle`,
- records the chain head after the real transfer,
- simulates another transfer through `POST /private/bundle/simulate`,
- verifies the block head and token balances did not change after simulation.

Run from the repository root:

```shell
scenarios/repeatable/bundle-simulation/run.zsh
```
