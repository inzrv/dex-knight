# Scenarios

Integration scenarios are grouped by local-environment lifecycle.
Pick the group first, then run the scenario inside it.

## Groups

- [`repeatable/`](repeatable/README.md): start or reuse local services and are safe to run multiple times.
- [`ready-env/`](ready-env/README.md): require an already prepared local environment.
- [`one-shot/`](one-shot/README.md): own a fresh local environment and clean it up afterward.

Run commands from the repository root unless a scenario says otherwise.
