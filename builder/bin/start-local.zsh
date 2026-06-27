#!/usr/bin/env zsh
set -euo pipefail

# Resolve paths relative to this script, so it can be called from any directory.
SCRIPT_DIR="${0:A:h}"
SERVICE_DIR="${SCRIPT_DIR:h}"
RUNTIME_DIR="${SERVICE_DIR}/runtime"
PID_FILE="${RUNTIME_DIR}/block-builder.local.pid"
LOG_FILE="${RUNTIME_DIR}/block-builder.local.log"
HOST="127.0.0.1"
PORT="9001"
HEALTH_URL="http://${HOST}:${PORT}/health"

wait_for_health() {
  for _ in {1..30}; do
    if curl -fsS "${HEALTH_URL}" >/dev/null 2>&1; then
      return 0
    fi

    sleep 0.2
  done

  echo "Block builder did not become healthy at ${HEALTH_URL}" >&2
  return 1
}

is_running() {
  local pid="$1"

  kill -0 "${pid}" >/dev/null 2>&1
}

mkdir -p "${RUNTIME_DIR}"
cd "${SERVICE_DIR}"

if [[ -f "${PID_FILE}" ]]; then
  EXISTING_PID="$(cat "${PID_FILE}")"
  if is_running "${EXISTING_PID}"; then
    echo "Block builder is already running with PID ${EXISTING_PID}"
    exit 0
  fi

  rm -f "${PID_FILE}"
fi

if curl -fsS "${HEALTH_URL}" >/dev/null 2>&1; then
  echo "Block builder already responds at ${HEALTH_URL}, but no managed PID file was found."
  echo "Stop the existing process manually or run cleanup-local.zsh if it was started by this workflow."
  exit 1
fi

if [[ ! -d ".venv" ]]; then
  echo "Creating Python virtual environment"
  python3 -m venv .venv
fi

PYTHON_BIN="${SERVICE_DIR}/.venv/bin/python3"
if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "Python virtual environment is not runnable at ${PYTHON_BIN}" >&2
  exit 1
fi

if ! "${PYTHON_BIN}" -c "import fastapi, uvicorn" >/dev/null 2>&1; then
  echo "Installing block builder dependencies"
  "${PYTHON_BIN}" -m pip install "fastapi>=0.110" "uvicorn[standard]>=0.27"
fi

echo "Starting block builder at ${HEALTH_URL}"
PYTHONPATH="${SERVICE_DIR}${PYTHONPATH:+:${PYTHONPATH}}" "${PYTHON_BIN}" -m block_builder.main >"${LOG_FILE}" 2>&1 &

PID="$!"
echo "${PID}" >"${PID_FILE}"
disown "${PID}" >/dev/null 2>&1 || true

wait_for_health

echo "Block builder started with PID ${PID}"
echo "Log file: ${LOG_FILE}"
