# Knight

Knight is the C++ arbitrage bot runtime for DEX Knight. It is intentionally small for now: it reads local config, starts a long-running idle loop, and waits for future mempool/core logic.

## Config

`config.json` stores local runtime settings.

```json
{
  "builderRestUrl": "http://127.0.0.1:9001",
  "builderWsUrl": "ws://127.0.0.1:9001/ws/pending",
  "tlsVerifyPeer": true
}
```

`builderRestUrl` points to the Forest Gate HTTP API. `builderWsUrl` points to the pending transaction stream. Use `http://`/`ws://` locally and `https://`/`wss://` for TLS endpoints. `tlsVerifyPeer` is optional and defaults to `true`.

## Run With Local Scripts

From the repository root:

```shell
knight/bin/start-local.zsh
```

The script configures CMake, builds `knight`, starts it in the background, writes a PID file, writes a log file, and checks that the process did not exit immediately.

Runtime files:

```text
knight/runtime/knight.local.pid
knight/runtime/knight.local.log
```

Stop and clean the local bot runtime:

```shell
knight/bin/cleanup-local.zsh
```
