# Forest Gate

Forest Gate is the local block builder for DEX Knight. It keeps an in-memory
public mempool, accepts private bundles, talks to Anvil, and exposes small chain
gateway endpoints for Knight.

Detailed endpoint examples live in [`docs/endpoints.md`](docs/endpoints.md).

## Capabilities

- Public mempool ingestion through `POST /public/tx`.
- Pending transaction lookup through `GET /public/tx/{mempoolTxId}` and
  `GET /public/pending`.
- WebSocket pending stream through `WS /ws/pending`.
- Chain gateway endpoints for `GET /chain/head`, `GET /chain/nonce/{address}`,
  and generic read-only `POST /chain/call`.
- Bundle simulation through `POST /private/bundle/simulate`.
- Bundle mining through `POST /private/bundle`.

Bundle payloads are ordered lists. Each item can be a public mempool reference:

```json
{ "mempoolTxId": "mp-..." }
```

or a direct local transaction JSON object.

Simulation sends the bundle to Anvil, mines one temporary block, collects
receipts, and reverts the Anvil snapshot. Public mempool records are not marked
as mined during simulation.

Submission mines one real block at the current head. Public mempool records
referenced by the submitted bundle are marked with their final transaction
status and receipt data. The submit response itself is the current status check:
there is no persisted bundle history API yet.

## Run With Local Scripts

From the repository root:

```shell
services/block-builder/bin/start-local.zsh
```

The script creates `.venv` if needed, installs Python dependencies if needed, starts the
service from the local source tree in the background, writes a PID file, writes
a log file, and checks `/health`.

Runtime files:

```text
services/block-builder/runtime/block-builder.local.pid
services/block-builder/runtime/block-builder.local.log
```

Stop and clean the local service:

```shell
services/block-builder/bin/cleanup-local.zsh
```

## Planned Later

- Persisted bundle history/status storage.
- Configurable Anvil RPC URL instead of a hardcoded node address.
- Raw signed transactions and `eth_sendRawTransaction`.
