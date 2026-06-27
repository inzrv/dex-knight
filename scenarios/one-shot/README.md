# One-Shot Scenarios

One-shot scenarios own a fresh local environment for the duration of one run.
They refuse to run if managed local services are already running, then clean services after execution.

The current one-shot backrun scenario builds and submits the backrun bundle from
the scenario script itself. Use ready-env scenarios when testing Knight's runtime
loop.

Run from the repository root:

```shell
scenarios/one-shot/backrun/run.zsh
```
