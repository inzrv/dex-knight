# Actor Nonces Scenario

## Lifecycle

- Group: `ready-env`.
- Read-only; does not mine blocks, submit transactions, or mutate mempool state.
- Does not start, redeploy, or clean local services.
- Prepare a clean environment with `scenarios/ready-env/bin/start-clean.zsh` and clean it with `scenarios/ready-env/bin/cleanup.zsh`.

This scenario checks the block-builder nonce endpoint for deployment actors.

It

- reads the existing local deployment metadata,
- checks that the local chain RPC and block builder are reachable,
- requests nonce through `GET /chain/nonce/{address}` for each deployment role,
- compares each builder nonce with the direct RPC nonce.

Run from the repository root:

```shell
scenarios/ready-env/actor-nonces/run.zsh
```
