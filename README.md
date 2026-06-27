```
    ____  _______  __    __ __       _       __    __ 
   / __ \/ ____/ |/ /   / //_/____  (_)___ _/ /_  / /_
  / / / / __/  |   /   / ,<  / __ \/ / __ `/ __ \/ __/
 / /_/ / /___ /   |   / /| |/ / / / / /_/ / / / / /_  
/_____/_____//_/|_|  /_/ |_/_/ /_/_/\__, /_/ /_/\__/  
                                   /____/
```
# DEX Knight

DEX Knight is a DEX arbitrage bot project with a local execution environment
for developing and testing the full workflow safely. The repository includes
the `blockchain` sandbox EVM network, a local `builder`,
smart contracts, and integration scenarios that exercise victim swaps, bundles,
simulations, and backruns.

## Current MVP

The local stack can run the first end-to-end backrun loop:

- The builder accepts public mempool transactions, streams pending updates,
  exposes chain head/nonce/`eth_call` gateway endpoints, simulates bundles on an
  Anvil snapshot, and mines submitted bundles at the current head.
- Knight watches configured sandbox pools, snapshots pool reserves on each new
  block, decodes pending sandbox swaps, calculates a two-pool `B -> A -> B`
  opportunity, composes `SandboxBackrun.executeBackrun`, simulates the ordered
  `[victim, backrun]` bundle, and submits it when simulation succeeds.
- Scenarios prepare deterministic local liquidity, approvals, actor balances,
  pending victim swaps, bundle simulation, bundle submission, and manual
  one-shot backruns.

## Modules

- `blockchain/` - local EVM network, ERC-20 tokens, AMM pools, backrun
  executor, and Foundry deployment scripts.
- `builder/` - local `builder` service with
  public mempool, private bundle, simulation, and chain gateway APIs.
- `knight/` - **Knight**: C++ arbitrage bot runtime.
- `scenarios/` - integration scenarios grouped by environment lifecycle.

## Stack

- Solidity contracts.
- Foundry: `anvil`, `forge`, `cast`.
- Python, FastAPI, Uvicorn.
- C++23, CMake, Boost, OpenSSL, spdlog, solabi.
- zsh local workflow scripts.

## Local Commands

### Blockchain

Start or redeploy the local blockchain sandbox:

```shell
blockchain/bin/deploy-local.zsh
```

Stop and clean the local blockchain sandbox:

```shell
blockchain/bin/cleanup-local.zsh
```

### Builder

Start the local block builder:

```shell
builder/bin/start-local.zsh
```

Stop and clean the local block builder:

```shell
builder/bin/cleanup-local.zsh
```

### Knight

Build and start the bot runtime:

```shell
knight/bin/start-local.zsh
```

Stop and clean the bot runtime:

```shell
knight/bin/cleanup-local.zsh
```

### Scenarios

Scenarios are grouped by lifecycle. See `scenarios/README.md` for the full list.

Run a repeatable smoke scenario:

```shell
scenarios/repeatable/token-transfer/run.zsh
```

Run the one-shot backrun scenario against a fresh environment:

```shell
scenarios/one-shot/backrun/run.zsh
```

Prepare a clean ready-env workspace, run a scenario that expects services to be running, then clean it up:

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/single-bundle-tx/run.zsh
scenarios/ready-env/bin/cleanup.zsh
```

Run the current Knight backrun loop locally:

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/fund-actor-tokens/run.zsh
scenarios/ready-env/approve-victim-tokens/run.zsh
scenarios/ready-env/seed-pools/run.zsh
scenarios/ready-env/victim-swap-pending/run.zsh
tail -f knight/runtime/knight.local.log
scenarios/ready-env/bin/cleanup.zsh
```

`start-clean.zsh` starts Knight by default. Use `START_KNIGHT=0` when testing
only the chain and block builder.

## More Detail

- `blockchain/README.md` - local chain, contracts, deployment output, and
  Foundry usage.
- `builder/README.md` - builder setup, service runtime, and
  API examples.
- `knight/README.md` - Knight bot build, config, and runtime scripts.
