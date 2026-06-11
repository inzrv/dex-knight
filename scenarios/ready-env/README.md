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

Run from the repository root:

```shell
scenarios/ready-env/actor-nonces/run.zsh
scenarios/ready-env/fund-victim-tokens/run.zsh
scenarios/ready-env/approve-victim-tokens/run.zsh
scenarios/ready-env/seed-pools/run.zsh
scenarios/ready-env/victim-swap-pending/run.zsh
scenarios/ready-env/flush-mempool/run.zsh
scenarios/ready-env/single-bundle-tx/run.zsh
```
