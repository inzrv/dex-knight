# Approve Victim Tokens Scenario

This scenario grants unlimited sandbox token approvals from the victim wallet to both local pools.
Run it before submitting victim transactions to the builder public mempool.
If a victim transaction is already pending, flush or clean the environment and resubmit it after approvals.

## Steps

- reads the existing local deployment metadata,
- checks that the local chain RPC is reachable,
- approves Pool1 and Pool2 to spend victim TokenA and TokenB,
- verifies all four allowances are exactly `uint256.max`.

## Run

```shell
scenarios/ready-env/approve-victim-tokens/run.zsh
```
