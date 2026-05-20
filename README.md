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
**Dark Forest**, a sandbox EVM network, **Forest Gate**, a local block builder,
smart contracts, and integration scenarios that exercise victim swaps, bundles,
simulations, and backruns.

## Modules

- `blockchain/` - **Dark Forest**: local EVM network, ERC-20 tokens, AMM
  pools, backrun executor, and Foundry deployment scripts.
- `services/block-builder/` - **Forest Gate**: local block builder service with
  public mempool, private bundle, simulation, and chain gateway APIs.
- `knight/` - **Knight**: C++ arbitrage bot runtime.
- `scenarios/` - integration scenarios that coordinate the chain and builder
  while the arbitrage bot is being built.

## Stack

- Solidity contracts.
- Foundry: `anvil`, `forge`, `cast`.
- Python, FastAPI, Uvicorn.
- C++23, CMake, Boost, OpenSSL, spdlog, solabi.
- zsh local workflow scripts.

## Local Commands

### Dark Forest

Start or redeploy the local blockchain sandbox:

```shell
blockchain/bin/deploy-local.zsh
```

Stop and clean the local blockchain sandbox:

```shell
blockchain/bin/cleanup-local.zsh
```

### Forest Gate

Start the local block builder:

```shell
services/block-builder/bin/start-local.zsh
```

Stop and clean the local block builder:

```shell
services/block-builder/bin/cleanup-local.zsh
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

Run the smoke scenario:

```shell
scenarios/token-transfer/run.zsh
```

The scenario starts or reuses the local chain and block builder, sends a `TokenA` transfer through the public mempool, mines it through a private bundle, and checks the final balance.

Seed both AMM pools:

```shell
scenarios/seed-pools/run.zsh
```

Run a successful victim swap through the public mempool:

```shell
scenarios/victim-swap/run.zsh
```

Run a victim swap that should revert on slippage:

```shell
scenarios/victim-swap-revert/run.zsh
```

Run a victim swap plus a simple backrun bundle:

```shell
scenarios/backrun/run.zsh
```

Check that bundle simulation returns receipts without changing chain state:

```shell
scenarios/bundle-simulation/run.zsh
```

Check public mempool sequence numbers and snapshot boundaries:

```shell
scenarios/mempool-sequence/run.zsh
```

Check that a single public self-transfer bundle produces exactly one block with one transaction:

```shell
scenarios/single-bundle-tx/run.zsh
```

## More Detail

- `blockchain/README.md` - Dark Forest local chain, contracts, deployment output, and
  Foundry usage.
- `services/block-builder/README.md` - Forest Gate setup, service runtime, and
  API examples.
- `knight/README.md` - Knight bot build, config, and runtime scripts.
