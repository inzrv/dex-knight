#!/usr/bin/env zsh
set -euo pipefail

# Start a fresh local environment for ready-env scenarios. By default this
# starts the chain, block builder, and Knight. Use START_KNIGHT=0 to skip Knight.
SCRIPT_DIR="${0:A:h}"
REPO_ROOT="${SCRIPT_DIR:h:h:h}"

"${SCRIPT_DIR}/cleanup.zsh"
"${REPO_ROOT}/blockchain/bin/deploy-local.zsh"
"${REPO_ROOT}/builder/bin/start-local.zsh"

if [[ "${START_KNIGHT:-1}" == "1" ]]; then
  "${REPO_ROOT}/knight/bin/start-local.zsh"
else
  echo "Skipping Knight startup because START_KNIGHT=0"
fi
