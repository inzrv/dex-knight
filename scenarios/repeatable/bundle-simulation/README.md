# Bundle Simulation Scenario

This scenario checks that bundle simulation produces execution results without
changing the real chain state.
It exercises `POST /private/bundle/simulate`, including the explicit
`blockNumber` guard used by Knight before submitting a real bundle.

## Steps

- starts or reuses the local chain and block builder,
- mints `3 TokenA` to the deployer,
- mines one real `TokenA` transfer through `POST /private/bundle`,
- records the chain head after the real transfer,
- simulates another transfer through `POST /private/bundle/simulate`,
- verifies the block head and token balances did not change after simulation.

## Run

```shell
scenarios/repeatable/bundle-simulation/run.zsh
```
