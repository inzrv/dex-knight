#!/usr/bin/env zsh
set -euo pipefail

# One-shot scenario wrapper: run only against a clean local environment and
# clean up services after the scenario finishes.
SCRIPT_DIR="${0:A:h}"
REPO_ROOT="${SCRIPT_DIR:h:h:h}"

pid_file_running() {
  local pid_file="$1"

  [[ -f "${pid_file}" ]] || return 1

  local pid
  pid="$(cat "${pid_file}")"
  kill -0 "${pid}" >/dev/null 2>&1
}

environment_running() {
  pid_file_running "${REPO_ROOT}/blockchain/deployments/anvil.local.pid" && return 0
  pid_file_running "${REPO_ROOT}/services/block-builder/runtime/block-builder.local.pid" && return 0
  pid_file_running "${REPO_ROOT}/knight/runtime/knight.local.pid" && return 0
  return 1
}

cleanup_environment() {
  "${REPO_ROOT}/knight/bin/cleanup-local.zsh" || true
  "${REPO_ROOT}/services/block-builder/bin/cleanup-local.zsh" || true
  "${REPO_ROOT}/blockchain/bin/cleanup-local.zsh" || true
}

if environment_running; then
  echo "One-shot scenarios require a clean local environment." >&2
  echo "Clean it first with: scenarios/ready-env/bin/cleanup.zsh" >&2
  exit 1
fi

trap cleanup_environment EXIT
python3 "${SCRIPT_DIR}/run.py" "$@"
