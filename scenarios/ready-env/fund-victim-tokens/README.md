# Fund Victim Tokens Scenario

This scenario mints sandbox tokens directly to the victim wallet.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- mints 1,000 TokenA and 1,000 TokenB to the victim account,
- verifies the victim balance increased by exactly 1,000 for both tokens.

## Run

```shell
scenarios/ready-env/fund-victim-tokens/run.zsh
```
