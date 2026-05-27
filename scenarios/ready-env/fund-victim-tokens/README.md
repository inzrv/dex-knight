# Fund Victim Tokens Scenario

## Lifecycle

- Group: `ready-env`.
- Does not start, redeploy, or clean local services.
- Prepare a clean environment with `scenarios/ready-env/bin/start-clean.zsh` and clean it with `scenarios/ready-env/bin/cleanup.zsh`.

This scenario mints sandbox tokens directly to the victim wallet.

It

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- mints 1,000 TokenA and 1,000 TokenB to the victim account,
- verifies the victim balance increased by exactly 1,000 for both tokens.

Run from the repository root:

```shell
scenarios/ready-env/fund-victim-tokens/run.zsh
```
