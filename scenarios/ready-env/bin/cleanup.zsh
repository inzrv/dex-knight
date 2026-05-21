#!/usr/bin/env zsh
set -euo pipefail

# Clean all local services used by ready-env scenarios.
SCRIPT_DIR="${0:A:h}"
REPO_ROOT="${SCRIPT_DIR:h:h:h}"

"${REPO_ROOT}/knight/bin/cleanup-local.zsh" || true
"${REPO_ROOT}/services/block-builder/bin/cleanup-local.zsh" || true
"${REPO_ROOT}/blockchain/bin/cleanup-local.zsh" || true
