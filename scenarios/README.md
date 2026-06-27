# Scenarios

Integration scenarios are grouped by local-environment lifecycle.
Pick the group first, then run the scenario inside it.

## Groups

- [`repeatable/`](repeatable/README.md): start or reuse local services and are safe to run multiple times.
- [`ready-env/`](ready-env/README.md): require an already prepared local environment.
- [`one-shot/`](one-shot/README.md): own a fresh local environment and clean it up afterward.

Run commands from the repository root unless a scenario says otherwise.

## Current Knight Flow

For the current bot MVP, use ready-env scenarios:

```shell
scenarios/ready-env/bin/start-clean.zsh
scenarios/ready-env/fund-actor-tokens/run.zsh
scenarios/ready-env/approve-victim-tokens/run.zsh
scenarios/ready-env/seed-pools/run.zsh
scenarios/ready-env/victim-swap-pending/run.zsh
tail -f knight/runtime/knight.local.log
scenarios/ready-env/bin/cleanup.zsh
```

`start-clean.zsh` starts the chain, block builder, and Knight. The pending
victim swap scenario leaves the swap in Forest Gate's public mempool; Knight can
then calculate, simulate, submit, and log the backrun bundle.
