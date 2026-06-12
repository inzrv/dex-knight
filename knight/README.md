# Knight

Knight is the C++ arbitrage bot runtime for DEX Knight. It is intentionally small for now: it reads local config, starts a long-running idle loop, and waits for future mempool/core logic.

## Config

`config.json` stores local runtime settings.

```json
{
  "builderRestUrl": "http://127.0.0.1:9001",
  "builderWsUrl": "ws://127.0.0.1:9001/ws/pending",
  "tlsVerifyPeer": true,
  "backrun": {
    "botAddress": "0x3c44cdddb6a900fa2b585dd299e03d12fa4293bc",
    "contractAddress": "0xDc64a140Aa3E981100a9becA4E685f962f0cF6C9",
    "chainId": 31337,
    "gas": 700000,
    "maxFeePerGas": "0x77359400",
    "maxPriorityFeePerGas": "0x1"
  },
  "pools": [
    {
      "address": "0x...",
      "tokenA": "0x...",
      "tokenB": "0x..."
    }
  ]
}
```

`builderRestUrl` points to the Forest Gate HTTP API. `builderWsUrl` points to the pending transaction stream. Use `http://`/`ws://` locally and `https://`/`wss://` for TLS endpoints. `tlsVerifyPeer` is optional and defaults to `true`.

`botAddress` is the account whose nonce is fetched through the builder, `contractAddress` is the deployed `SandboxBackrun` executor.

`pools` lists AMM pools watched by the candidate filter; only swap transactions sent to these addresses are delivered to the local mempool.

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
