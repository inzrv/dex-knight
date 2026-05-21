#!/usr/bin/env zsh
set -euo pipefail

# Thin launcher kept for shell usage. The scenario body is Python because it is
# mostly JSON-RPC, HTTP, and structured validation.
SCRIPT_DIR="${0:A:h}"

python3 "${SCRIPT_DIR}/run.py" "$@"
