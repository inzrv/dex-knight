# Scenarios

Integration scenarios are grouped by their local-environment lifecycle. Pick the group first, then run the scenario inside it.

## Repeatable

`repeatable/` scenarios are safe to run multiple times. They check the local environment and start or reuse the services they need.

```shell
scenarios/repeatable/token-transfer/run.zsh
scenarios/repeatable/victim-swap/run.zsh
scenarios/repeatable/victim-swap-revert/run.zsh
scenarios/repeatable/bundle-simulation/run.zsh
scenarios/repeatable/mempool-sequence/run.zsh
scenarios/repeatable/seed-pools/run.zsh
```

## One-Shot

`one-shot/` scenarios own a fresh local environment for the duration of the run. They refuse to run if managed local services are already running, then clean up after execution.

```shell
scenarios/one-shot/backrun/run.zsh
```

## Ready Env

`ready-env/` scenarios never start or clean services by themselves. Use the shared scripts to prepare and clean a full local environment.

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/single-bundle-tx/run.zsh
scenarios/ready-env/bin/cleanup.zsh
```

Use `START_KNIGHT=0 scenarios/ready-env/bin/start-clean.zsh` when the scenario only needs the chain and block builder.
