# Knight

Knight is the C++23 arbitrage bot runtime for DEX Knight.

In the current local MVP it connects to Forest Gate, keeps a coherent view of
the watched sandbox pools, reacts to pending victim swaps, calculates a simple
two-pool backrun opportunity, simulates the ordered bundle, and submits the
bundle when simulation succeeds.

## Runtime Flow

At startup Knight:

1. Reads `config.json`.
2. Creates the candidate source, simulator, and backrun transaction composer.
3. Requests the bot nonce through the builder `GET /chain/nonce/{address}`
   endpoint.
4. Starts the block syncer and builder pending websocket feed.

During the core loop Knight:

1. Waits for a `candidate::StateSnapshot`.
2. Skips invalid snapshots and snapshots without a candidate.
3. Decodes sandbox DEX swap calldata.
4. Applies the victim swap to the local pool model.
5. Calculates the best current two-pool `B -> A -> B` opportunity.
6. Encodes `SandboxBackrun.executeBackrun(...)` and builds a direct private
   transaction from the configured bot address.
7. Builds a bundle with the public mempool victim transaction followed by the
   private backrun transaction.
8. Simulates the bundle through `POST /private/bundle/simulate`.
9. Submits the bundle through `POST /private/bundle` only when simulation status
   is `included`.
10. Logs bundle and per-transaction statuses, then advances the local bot nonce
    after a successful submit response.

The runtime is still intentionally sequential. There is no parallel simulation
queue, gas/profit policy, persisted strategy state, or real mempool ordering
model yet.

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
      "address": "0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0",
      "tokenA": "0x5FbDB2315678afecb367f032d93F642f64180aa3",
      "tokenB": "0xe7f1725E7734CE288F8367e1Bb143E90bb3F0512"
    },
    {
      "address": "0xCf7Ed3AccA5a467e9e704C703E8D87F634fB0Fc9",
      "tokenA": "0x5FbDB2315678afecb367f032d93F642f64180aa3",
      "tokenB": "0xe7f1725E7734CE288F8367e1Bb143E90bb3F0512"
    }
  ]
}
```

`builderRestUrl` points to the Forest Gate HTTP API. `builderWsUrl` points to
the pending transaction stream. Use `http://`/`ws://` locally and
`https://`/`wss://` for TLS endpoints. `tlsVerifyPeer` is optional and defaults
to `true`.

`backrun.botAddress` is the transaction sender and `SandboxBackrun` operator.
Knight fetches its nonce from Forest Gate at startup. `backrun.contractAddress`
is the deployed `SandboxBackrun` executor. Gas and EIP-1559 fee fields are used
for the direct private backrun transaction.

`pools` lists AMM pools watched by the candidate filter and used by the
arbitrage calculator. The local checked-in config matches the deterministic
first deployment, but redeploying contracts can change addresses. Update this
file from `blockchain/deployments/local.json` when needed.

## Local Strategy Assumptions

- Only sandbox DEX swap calldata is decoded.
- Current candidate filtering watches configured pool addresses and the
  `swapExactAForB` / `swapExactBForA` selectors.
- Pool reserves are snapshotted through builder `eth_call` on every new block.
- The calculator models two pools over the same TokenA/TokenB pair and searches
  `B -> A -> B` routes.
- Runtime params are deliberately permissive for MVP testing: large
  `max_amount_in_b`, zero `min_profit_b`, and zero `min_output_bps`.
- Transactions are local JSON transactions sent by Anvil unlocked accounts, not
  raw signed transactions.

## Build, Test, And Run

From the repository root:

```shell
knight/bin/start-local.zsh
```

The script:

1. Runs `clang-format` over `knight/src` and `knight/tests`.
2. Configures CMake with `BUILD_TESTING=ON`.
3. Builds `knight_tests`.
4. Runs `ctest`.
5. Builds `knight`.
6. Starts the runtime in the background.

Runtime files:

```text
knight/runtime/knight.local.pid
knight/runtime/knight.local.log
```

Stop and clean the local bot runtime:

```shell
knight/bin/cleanup-local.zsh
```

Manual build and test commands:

```shell
cmake -S knight -B knight/build -DBUILD_TESTING=ON
cmake --build knight/build
ctest --test-dir knight/build --output-on-failure
```
