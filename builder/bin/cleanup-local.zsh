#!/usr/bin/env zsh
set -euo pipefail

# Resolve paths relative to this script, so it can be called from any directory.
SCRIPT_DIR="${0:A:h}"
SERVICE_DIR="${SCRIPT_DIR:h}"
RUNTIME_DIR="${SERVICE_DIR}/runtime"
PID_FILE="${RUNTIME_DIR}/block-builder.local.pid"
LOG_FILE="${RUNTIME_DIR}/block-builder.local.log"

stop_process() {
  local pid="$1"

  if ! kill -0 "${pid}" >/dev/null 2>&1; then
    echo "No running process found for saved block-builder PID ${pid}"
    return 0
  fi

  # Avoid killing an unrelated process if the PID file is stale.
  if ! ps -p "${pid}" -o command= | grep -Eq "block-builder|block_builder|uvicorn"; then
    echo "Saved PID ${pid} does not look like a block-builder process; leaving it untouched"
    return 0
  fi

  echo "Stopping block builder process ${pid}"
  kill "${pid}"

  for _ in {1..20}; do
    if ! kill -0 "${pid}" >/dev/null 2>&1; then
      return 0
    fi

    sleep 0.2
  done

  echo "Block builder process ${pid} did not exit after SIGTERM; sending SIGKILL"
  kill -9 "${pid}" >/dev/null 2>&1 || true
}

if [[ -f "${PID_FILE}" ]]; then
  PID="$(cat "${PID_FILE}")"
  stop_process "${PID}"
else
  echo "No local block-builder PID file found"
fi

rm -f "${PID_FILE}"
rm -f "${LOG_FILE}"

echo "Local block-builder environment cleaned up"
