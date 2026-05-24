# Victim Swap Pending Scenario

## Lifecycle

- Group: `ready-env`.
- Does not start, redeploy, or clean local services.
- Prepare a clean environment with `scenarios/ready-env/bin/start-clean.zsh` and clean it with `scenarios/ready-env/bin/cleanup.zsh`.

This scenario submits one victim `swapExactAForB` transaction to `Pool1` through
the builder public mempool and deliberately does not mine a block.

It

- reads the existing local deployment metadata,
- checks that the local chain RPC and block builder are reachable,
- checks that Anvil automine is disabled,
- submits a `swapExactAForB(100 TokenA, 0 min TokenB)` transaction to `Pool1`,
- verifies the chain head did not change,
- verifies the builder public mempool contains one additional pending transaction.

Run from the repository root:

```shell
scenarios/ready-env/victim-swap-pending/run.zsh
```
