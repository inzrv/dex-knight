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
- `scenarios/` - integration scenarios grouped by environment lifecycle.

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

## More Detail

- `blockchain/README.md` - Dark Forest local chain, contracts, deployment output, and
  Foundry usage.
- `services/block-builder/README.md` - Forest Gate setup, service runtime, and
  API examples.
- `knight/README.md` - Knight bot build, config, and runtime scripts.
