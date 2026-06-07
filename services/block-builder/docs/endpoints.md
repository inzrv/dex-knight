# Block Builder Endpoints

The local block builder runs on `http://127.0.0.1:9001` by default.

Example contract addresses are from one local sandbox deployment. Fresh
deployments can produce different addresses.

## Overview

```text
GET  /health
GET  /ping
GET  /chain/head
POST /chain/call
POST /public/tx
GET  /public/tx/{mempoolTxId}
GET  /public/pending
POST /private/bundle/simulate
POST /private/bundle
WS   /ws/pending
```

## `GET /health`

Description: checks that the service is running.

Example request:

```shell
curl http://127.0.0.1:9001/health
```

Expected response:

```json
{
  "status": "ok"
}
```

## `GET /ping`

Description: lightweight connectivity check.

Example request:

```shell
curl http://127.0.0.1:9001/ping
```

Expected response:

```json
{
  "message": "pong"
}
```

## `GET /chain/head`

Description: returns the latest chain head metadata as seen by the builder.

Example request:

```shell
curl http://127.0.0.1:9001/chain/head
```

Expected response:

```json
{
  "blockNumber": "0x1",
  "blockHash": "0xabc...",
  "parentHash": "0xdef...",
  "timestamp": "0x663f5a10",
  "baseFeePerGas": "0x3b9aca00"
}
```

## `POST /chain/call`

Description: executes a generic read-only `eth_call` through the builder. The
builder does not know contract ABIs; callers provide the target address and
calldata.

Example request: read `getReserves()` from `Pool1`.

```shell
curl -X POST http://127.0.0.1:9001/chain/call \
  -H "Content-Type: application/json" \
  -d '{
    "to": "0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0",
    "data": "0x0902f1ac",
    "block": "latest"
  }'
```

Expected response:

```json
{
  "block": "latest",
  "result": "0x00000000000000000000000000000000000000000000003635c9adc5dea00000000000000000000000000000000000000000000000000003635c9adc5dea00000"
}
```

The encoded `result` above is `(1000e18, 1000e18)` for
`getReserves()(uint256,uint256)`.

## `POST /public/tx`

Description: adds a transaction payload to the builder's in-memory public
mempool and broadcasts it to websocket subscribers.

Example request:

```shell
curl -X POST http://127.0.0.1:9001/public/tx \
  -H "Content-Type: application/json" \
  -d '{
    "type": "0x2",
    "chainId": "0x7a69",
    "nonce": "0x0",
    "from": "0xa0Ee7A142d267C1f36714E4a8F75612F20a79720",
    "to": "0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f",
    "value": "0x1",
    "gas": "0x493e0",
    "maxFeePerGas": "0x77359400",
    "maxPriorityFeePerGas": "0x1",
    "input": "0x"
  }'
```

Expected response:

```json
{
  "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
  "seqNum": 42,
  "status": "pending",
  "chainTxHash": null,
  "transaction": {
    "type": "0x2",
    "chainId": "0x7a69",
    "nonce": "0x0",
    "from": "0xa0Ee7A142d267C1f36714E4a8F75612F20a79720",
    "to": "0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f",
    "value": "0x1",
    "gas": "0x493e0",
    "maxFeePerGas": "0x77359400",
    "maxPriorityFeePerGas": "0x1",
    "input": "0x"
  },
  "receipt": null
}
```

## `GET /public/tx/{mempoolTxId}`

Description: reads one public mempool record by its generated id.

Example request:

```shell
curl http://127.0.0.1:9001/public/tx/mp-1234567890abcdef1234567890abcdef
```

Expected response:

```json
{
  "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
  "seqNum": 42,
  "status": "pending",
  "chainTxHash": null,
  "transaction": {
    "type": "0x2",
    "chainId": "0x7a69",
    "nonce": "0x0",
    "from": "0xa0Ee7A142d267C1f36714E4a8F75612F20a79720",
    "to": "0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f",
    "value": "0x1",
    "gas": "0x493e0",
    "maxFeePerGas": "0x77359400",
    "maxPriorityFeePerGas": "0x1",
    "input": "0x"
  },
  "receipt": null
}
```

## `GET /public/pending`

Description: lists public mempool records that still have `pending` status.

Example request:

```shell
curl http://127.0.0.1:9001/public/pending
```

Expected response:

```json
{
  "snapshotSeq": 42,
  "transactions": [
    {
      "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
      "seqNum": 42,
      "status": "pending",
      "chainTxHash": null,
      "transaction": {
        "type": "0x2",
        "chainId": "0x7a69",
        "nonce": "0x0",
        "from": "0xa0Ee7A142d267C1f36714E4a8F75612F20a79720",
        "to": "0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f",
        "value": "0x1",
        "gas": "0x493e0",
        "maxFeePerGas": "0x77359400",
        "maxPriorityFeePerGas": "0x1",
        "input": "0x"
      },
      "receipt": null
    }
  ]
}
```

## `POST /private/bundle/simulate`

Description: simulates a bundle with the same payload shape as
`POST /private/bundle`. The builder executes the ordered transaction sequence on
an Anvil snapshot, collects receipts, and reverts the snapshot afterwards. Public
mempool records are not marked as mined during simulation.

Optional request field:

- `blockNumber`: hex quantity for the chain head/state block the caller expects
  the simulation to run against. If omitted, the builder uses the current head.
  Future block numbers are rejected. Historical block simulation requires an
  archived state execution path and is currently rejected instead of being
  silently simulated on the wrong state.

Example request:

```shell
curl -X POST http://127.0.0.1:9001/private/bundle/simulate \
  -H "Content-Type: application/json" \
  -d '{
    "blockNumber": "0x1",
    "transactions": [
      {
        "mempoolTxId": "mp-1234567890abcdef1234567890abcdef"
      },
      {
        "type": "0x2",
        "chainId": "0x7a69",
        "nonce": "0x0",
        "from": "0x3c44cdddb6a900fa2b585dd299e03d12fa4293bc",
        "to": "0xDc64a140Aa3E981100a9becA4E685f962f0cF6C9",
        "value": "0x0",
        "gas": "0xaae60",
        "maxFeePerGas": "0x77359400",
        "maxPriorityFeePerGas": "0x1",
        "input": "0x..."
      }
    ]
  }'
```

Expected response:

```json
{
  "status": "included",
  "simulated": true,
  "transactions": [
    {
      "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
      "chainTxHash": "0xabc...",
      "status": "included",
      "blockNumber": "0x2",
      "transactionIndex": "0x0",
      "receipt": {
        "status": "0x1"
      }
    },
    {
      "mempoolTxId": null,
      "chainTxHash": "0xdef...",
      "status": "included",
      "blockNumber": "0x2",
      "transactionIndex": "0x1",
      "receipt": {
        "status": "0x1"
      }
    }
  ]
}
```

## `POST /private/bundle`

Description: mines a block with the submitted bundle. A bundle can reference
public mempool transactions by `mempoolTxId` and can include direct private
transactions.

Optional request field:

- `blockNumber`: hex quantity for the current chain head. If omitted, the
  builder uses the current head. Mining rejects any value that does not equal
  the current head because the live chain can only execute from its latest
  state.

Example request:

```shell
curl -X POST http://127.0.0.1:9001/private/bundle \
  -H "Content-Type: application/json" \
  -d '{
    "blockNumber": "0x1",
    "transactions": [
      {
        "mempoolTxId": "mp-1234567890abcdef1234567890abcdef"
      },
      {
        "type": "0x2",
        "chainId": "0x7a69",
        "nonce": "0x0",
        "from": "0x3c44cdddb6a900fa2b585dd299e03d12fa4293bc",
        "to": "0xDc64a140Aa3E981100a9becA4E685f962f0cF6C9",
        "value": "0x0",
        "gas": "0xaae60",
        "maxFeePerGas": "0x77359400",
        "maxPriorityFeePerGas": "0x1",
        "input": "0x..."
      }
    ]
  }'
```

Expected response:

```json
{
  "status": "included",
  "transactions": [
    {
      "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
      "chainTxHash": "0xabc...",
      "status": "included",
      "blockNumber": "0x2",
      "transactionIndex": "0x0",
      "receipt": {
        "status": "0x1"
      }
    },
    {
      "mempoolTxId": null,
      "chainTxHash": "0xdef...",
      "status": "included",
      "blockNumber": "0x2",
      "transactionIndex": "0x1",
      "receipt": {
        "status": "0x1"
      }
    }
  ]
}
```

## `WS /ws/pending`

Description: streams newly submitted public mempool records to websocket
subscribers.

Example request:

```shell
wscat -c ws://127.0.0.1:9001/ws/pending
```

Expected message:

```json
{
  "type": "pending_transaction",
  "record": {
    "mempoolTxId": "mp-1234567890abcdef1234567890abcdef",
    "seqNum": 42,
    "status": "pending",
    "chainTxHash": null,
    "transaction": {
      "type": "0x2",
      "chainId": "0x7a69",
      "nonce": "0x0",
      "from": "0xa0Ee7A142d267C1f36714E4a8F75612F20a79720",
      "to": "0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f",
      "value": "0x1",
      "gas": "0x493e0",
      "maxFeePerGas": "0x77359400",
      "maxPriorityFeePerGas": "0x1",
      "input": "0x"
    },
    "receipt": null
  }
}
```
