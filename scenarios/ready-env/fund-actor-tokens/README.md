# Fund Actor Tokens Scenario

This scenario mints sandbox tokens directly to the victim and bot wallets.
It also gives `SandboxBackrun` TokenB starting capital for Knight's current
`B -> A -> B` backrun route.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- mints 1,000 TokenA and 1,000 TokenB to the victim account,
- mints 1,000 TokenA and 1,000 TokenB to the bot account,
- mints 1,000 TokenB to the backrun contract (starting capital for B → A → B routes),
- verifies each balance increased by exactly 1,000.

## Run

```shell
scenarios/ready-env/fund-actor-tokens/run.zsh
```
