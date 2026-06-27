# Blockchain

`blockchain/` is the local EVM sandbox for DEX Knight. This directory contains
the chain tooling, Solidity contracts, deployment scripts, and local deployment
outputs.

For now, it is intentionally small. The goal is to create a reproducible local
environment where the C++ bot can interact with contracts without using a
public RPC endpoint or spending real gas.

Current contents:

- `src/tokens/` - minimal ERC-20 contracts used by the sandbox.
- `src/dexes/` - sandbox AMM pool contracts used by swap and arbitrage
  scenarios.
- `src/backrun/` - sandbox backrun executor contracts for bundle-driven
  arbitrage experiments.
- `config/local.anvil.env` - local chain accounts, keys, RPC URL, and Anvil
  parameters.
- `bin/deploy-local.zsh` - starts or reuses a local Anvil chain, builds contracts, deploys the sandbox tokens and pools, checks them, and writes deployment output.
- `bin/cleanup-local.zsh` - stops the Anvil process started by the deploy
  script and removes local runtime files.
- `deployments/` - local runtime output such as deployed addresses, Anvil logs,
  and Anvil PID files. These files are ignored by Git.

## Local Workflow

From the repository root:

```shell
blockchain/bin/deploy-local.zsh
```

This creates a ready local blockchain environment:

1. Starts `anvil` if no local chain is running at `RPC_URL`.
2. Verifies that the local chain responds by reading the current block number.
3. Runs `forge build`.
4. Deploys `TokenA` and `TokenB`.
5. Deploys `Pool1` and `Pool2` over the `TokenA` / `TokenB` pair.
6. Deploys `SandboxBackrun` with the local bot account as its operator.
7. Reads token symbols, pool token addresses, initial pool reserves, and the backrun operator.
8. Saves deployment output and local role addresses/keys to `blockchain/deployments/local.json`.

Example output file:

```json
{
  "chainId": 31337,
  "rpcUrl": "http://127.0.0.1:8545",
  "deployer": "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266",
  "roles": {
    "deployer": {
      "address": "0xf39f...",
      "privateKey": "0xac09..."
    },
    "victim": {
      "address": "0x7099...",
      "privateKey": "0x59c6..."
    },
    "bot": {
      "address": "0x3c44...",
      "privateKey": "0x5de4..."
    },
    "treasury": {
      "address": "0x90f7..."
    }
  },
  "contracts": {
    "tokenA": "0x...",
    "tokenB": "0x...",
    "pool1": "0x...",
    "pool2": "0x...",
    "backrun": "0x..."
  },
  "checks": {
    "tokenASymbol": "TKA",
    "tokenBSymbol": "TKB",
    "pool1TokenA": "0x...",
    "pool1TokenB": "0x...",
    "pool1ReserveA": "0",
    "pool1ReserveB": "0",
    "pool2TokenA": "0x...",
    "pool2TokenB": "0x...",
    "pool2ReserveA": "0",
    "pool2ReserveB": "0",
    "backrunOperator": "0x3c44..."
  }
}
```

To stop and clean the local environment:

```shell
blockchain/bin/cleanup-local.zsh
```

The cleanup script removes:

- `blockchain/deployments/local.json`
- `blockchain/deployments/anvil.local.log`
- `blockchain/deployments/anvil.local.pid`

It only stops the Anvil process recorded in `anvil.local.pid`, and it checks
that the saved PID belongs to an `anvil` process before killing it.

## Running Twice

If `deploy-local.zsh` sees a running chain at `RPC_URL`, it checks whether that
chain was started by this workflow. If the saved PID is missing or does not
belong to `anvil`, the script refuses to deploy.

If the running chain is ours, the script asks for confirmation before deploying
new contracts. Re-running deployment on the same chain creates new token and pool contracts and overwrites `deployments/local.json`.

For a fresh environment:

```shell
blockchain/bin/cleanup-local.zsh
blockchain/bin/deploy-local.zsh
```

## Tools

This directory uses Foundry:

- `anvil` - local EVM node.
- `forge` - Solidity compiler, test runner, and deployment helper.
- `cast` - command-line JSON-RPC and contract interaction tool.

Useful manual commands:

```shell
cd blockchain
forge build
cast block-number --rpc-url http://127.0.0.1:8545
```

## Local Parameters

Local chain settings live in `config/local.anvil.env`.

Network settings:

- `RPC_URL` - local JSON-RPC endpoint.
- `CHAIN_ID` - local chain ID, currently `31337`.
- `HARD_FORK` - Anvil EVM hardfork, currently pinned to `prague`.
- `BASE_FEE_WEI` - initial EIP-1559 base fee, currently `1000000000`.
- `GAS_LIMIT` - block gas limit, currently `30000000`.
- `MNEMONIC` - deterministic Anvil mnemonic.
- `ACCOUNTS` - number of generated local accounts.
- `BALANCE_ETH` - starting ETH balance for each generated account.

Role settings:

- `DEPLOYER_ADDRESS` / `DEPLOYER_PRIVATE_KEY` - deploys contracts and is the
  current token minter.
- `VICTIM_ADDRESS` / `VICTIM_PRIVATE_KEY` - used by victim swap scenarios.
- `BOT_ADDRESS` / `BOT_PRIVATE_KEY` - used by Knight as the local backrun
  transaction sender and `SandboxBackrun` operator.
- `TREASURY_ADDRESS` - reserved for future profit collection.

These keys are deterministic Anvil development keys. They must never be used on
real networks.

## Contracts

Current token contracts:

- `SandboxERC20` - minimal ERC-20 implementation with a single `minter`.
- `TokenA` - sandbox token with symbol `TKA`.
- `TokenB` - sandbox token with symbol `TKB`.

Current DEX contracts:

- `SandboxDex` - minimal constant-product AMM with `TokenA` / `TokenB`
  reserves, 0.3% swap fee, bootstrap liquidity, and exact-input swaps.
- `Pool1` - first sandbox pool instance for local scenarios.
- `Pool2` - second sandbox pool instance for local scenarios and current
  two-pool arbitrage setups.

Current backrun contracts:

- `SandboxBackrun` - executes a simple `B -> A` swap in one pool followed by
  `A -> B` in another pool, then reverts if the realized `TokenB` profit is
  below a caller-provided minimum. It emits `BackrunExecuted` with the selected
  pools, input, realized outputs, and profit.

The contracts avoid external dependencies for now, so the sandbox can compile
without installing packages.
