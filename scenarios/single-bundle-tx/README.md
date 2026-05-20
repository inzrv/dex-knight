# Single Bundle Transaction Scenario

This scenario checks the smallest builder-controlled mining path: one public
mempool transaction is included through one private bundle and produces exactly
one new block with exactly one transaction.

It does not deploy contracts, seed pools, mint tokens, or restart services. The
local chain and block builder must already be running.

It

- reads the existing local deployment metadata,
- checks that the local chain RPC and block builder are reachable,
- checks that Anvil automine is disabled,
- submits a 0 ETH self-transfer from the victim account to the public mempool,
- mines a private bundle containing only that mempool transaction,
- verifies the chain advanced by exactly one block,
- verifies the new block contains exactly one transaction.

Run from the repository root:

```shell
scenarios/single-bundle-tx/run.zsh
```
