# Token Transfer Scenario

## Lifecycle

- Group: `repeatable`.
- Can be run multiple times against the same local environment.
- Starts or reuses the local chain and block builder when needed.

This scenario is a smoke test for the local blockchain and block builder.

It

- starts or reuses the local chain, 
- starts or reuses the block builder, 
- mints `100 TokenA` to the deployer,
- submits a `TokenA` transfer through the public mempool,
- mines it through the private bundle endpoint, 
- verifies the recipient balance.

Run from the repository root:

```shell
scenarios/repeatable/token-transfer/run.zsh
```
