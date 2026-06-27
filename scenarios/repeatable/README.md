# Repeatable Scenarios

Repeatable scenarios are safe to run multiple times against the same local environment.
They start or reuse the local chain and block builder when needed, unless a scenario says otherwise.

They may still mutate chain or builder state as part of their checks.
Most repeatable scenarios exercise the chain and builder directly; they do
not require Knight to be running.

Run from the repository root:

```shell
scenarios/repeatable/token-transfer/run.zsh
scenarios/repeatable/victim-swap/run.zsh
scenarios/repeatable/victim-swap-revert/run.zsh
scenarios/repeatable/bundle-simulation/run.zsh
scenarios/repeatable/mempool-sequence/run.zsh
```
