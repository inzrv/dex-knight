# Victim Swap Scenario

This scenario checks a successful victim swap through the public mempool and a
private bundle.

## Steps

- starts or reuses the local chain and block builder,
- ensures `Pool1` has liquidity, mints `100 TokenA` to the victim,
- approves `Pool1`, submits `swapExactAForB` through the public mempool,
- mines it through the private bundle endpoint,
- verifies the victim spent exactly `100 TokenA` and received at least the requested minimum `TokenB`.

## Run

```shell
scenarios/repeatable/victim-swap/run.zsh
```
