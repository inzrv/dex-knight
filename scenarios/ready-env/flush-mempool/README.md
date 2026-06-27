# Flush Mempool Scenario

This scenario reads every currently pending transaction from the builder public
mempool and sends them to the chain as one private bundle.
It is useful after pending-transaction experiments when you want the builder
mempool to become empty again without restarting services.

It does not require every transaction to succeed at EVM level. Reverted
transactions are acceptable; the goal is that they stop being pending in the
builder mempool.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC and block builder are reachable,
- checks that Anvil automine is disabled,
- reads `GET /public/pending`,
- submits one `POST /private/bundle` containing all pending `mempoolTxId`s,
- verifies `GET /public/pending` returns zero transactions after the bundle.

## Run

```shell
scenarios/ready-env/flush-mempool/run.zsh
```
