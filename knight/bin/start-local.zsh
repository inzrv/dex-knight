#!/usr/bin/env zsh
set -euo pipefail

# Resolve paths relative to this script, so it can be called from any directory.
SCRIPT_DIR="${0:A:h}"
KNIGHT_DIR="${SCRIPT_DIR:h}"
BUILD_DIR="${KNIGHT_DIR}/build"
RUNTIME_DIR="${KNIGHT_DIR}/runtime"
PID_FILE="${RUNTIME_DIR}/knight.local.pid"
LOG_FILE="${RUNTIME_DIR}/knight.local.log"
CONFIG_FILE="${KNIGHT_DIR}/config.json"
BINARY="${BUILD_DIR}/src/knight"

is_running() {
  local pid="$1"

  kill -0 "${pid}" >/dev/null 2>&1
}

run_clang_format() {
  if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found in PATH" >&2
    exit 1
  fi

  local -a format_dirs
  format_dirs=("${KNIGHT_DIR}/src")
  if [[ -d "${KNIGHT_DIR}/tests" ]]; then
    format_dirs+=("${KNIGHT_DIR}/tests")
  fi

  find "${format_dirs[@]}" \
    -type f \
    \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \) \
    -exec clang-format -i {} +
}

mkdir -p "${RUNTIME_DIR}"
cd "${KNIGHT_DIR}"

if [[ -f "${PID_FILE}" ]]; then
  EXISTING_PID="$(cat "${PID_FILE}")"
  if is_running "${EXISTING_PID}"; then
    echo "Knight is already running with PID ${EXISTING_PID}"
    exit 0
  fi

  rm -f "${PID_FILE}"
fi

if [[ ! -f "${CONFIG_FILE}" ]]; then
  echo "Knight config not found: ${CONFIG_FILE}" >&2
  exit 1
fi

echo "Formatting Knight sources"
run_clang_format

echo "Building Knight"
cmake -S "${KNIGHT_DIR}" -B "${BUILD_DIR}" -DBUILD_TESTING=ON
cmake --build "${BUILD_DIR}" --target knight_tests

echo "Running Knight tests"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

cmake --build "${BUILD_DIR}" --target knight

if [[ ! -x "${BINARY}" ]]; then
  echo "Knight binary was not built at ${BINARY}" >&2
  exit 1
fi

echo "Starting Knight with config ${CONFIG_FILE}"
nohup "${BINARY}" "${CONFIG_FILE}" >"${LOG_FILE}" 2>&1 &

PID="$!"
echo "${PID}" >"${PID_FILE}"
disown "${PID}" >/dev/null 2>&1 || true

sleep 0.5

if ! is_running "${PID}"; then
  rm -f "${PID_FILE}"
  echo "Knight exited immediately. Log file: ${LOG_FILE}" >&2
  exit 1
fi

echo "Knight started with PID ${PID}"
echo "Log file: ${LOG_FILE}"
