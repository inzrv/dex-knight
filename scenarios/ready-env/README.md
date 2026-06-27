# Ready Env Scenarios

Ready-env scenarios require an existing local deployment and running local services.
They do not start, redeploy, or clean services by themselves.

Prepare and clean the shared environment with:

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/bin/cleanup.zsh
```

Use `START_KNIGHT=0 scenarios/ready-env/bin/start-clean.zsh` when a scenario only needs the
chain and block builder.

## Bot MVP Flow

This sequence prepares the sandbox state that Knight currently expects:

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/fund-actor-tokens/run.zsh
scenarios/ready-env/approve-victim-tokens/run.zsh
scenarios/ready-env/seed-pools/run.zsh
scenarios/ready-env/victim-swap-pending/run.zsh
tail -f knight/runtime/knight.local.log
```

The funding step gives the backrun contract TokenB starting capital. The seed
step gives both pools equal liquidity. The pending victim swap creates the
public mempool candidate that Knight can backrun.

Clean up afterward:

```shell
scenarios/ready-env/bin/cleanup.zsh
```

Run from the repository root:

```shell
scenarios/ready-env/actor-nonces/run.zsh
scenarios/ready-env/fund-actor-tokens/run.zsh
scenarios/ready-env/approve-victim-tokens/run.zsh
scenarios/ready-env/seed-pools/run.zsh
scenarios/ready-env/victim-swap-pending/run.zsh
scenarios/ready-env/flush-mempool/run.zsh
scenarios/ready-env/single-bundle-tx/run.zsh
```
