# Actor Nonces Scenario

This scenario checks the block-builder nonce endpoint for deployment actors.
It is read-only: it does not mine blocks, submit transactions, or mutate mempool state.
Knight uses the same endpoint to initialize the bot sender nonce before building
private backrun transactions.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC and block builder are reachable,
- requests nonce through `GET /chain/nonce/{address}` for each deployment role,
- compares each builder nonce with the direct RPC nonce.

## Run

```shell
scenarios/ready-env/actor-nonces/run.zsh
```
