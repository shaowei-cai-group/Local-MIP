#!/bin/bash
# =============================================================================
# Build Local-MIP core (if missing) and Python bindings (pybind11)
# Output: python-bindings/build/localmip_py.*.so
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIND_DIR="${REPO_ROOT}/python-bindings"
BIND_BUILD="${BIND_DIR}/build"
CORE_BUILD="${REPO_ROOT}/build"

detect_python_executable() {
  if [ -n "${PYTHON_EXECUTABLE:-}" ]; then
    printf '%s\n' "${PYTHON_EXECUTABLE}"
    return
  fi

  if [ -n "${VIRTUAL_ENV:-}" ] && [ -x "${VIRTUAL_ENV}/bin/python" ]; then
    printf '%s\n' "${VIRTUAL_ENV}/bin/python"
    return
  fi

  if [ -n "${CONDA_PREFIX:-}" ] && [ -x "${CONDA_PREFIX}/bin/python" ]; then
    printf '%s\n' "${CONDA_PREFIX}/bin/python"
    return
  fi

  if command -v python3 >/dev/null 2>&1; then
    command -v python3
    return
  fi

  echo "Error: python3 not found. Set PYTHON_EXECUTABLE explicitly." >&2
  exit 1
}

PY_EXE="$(detect_python_executable)"

if [ ! -x "${PY_EXE}" ]; then
  echo "Error: Python executable is not executable: ${PY_EXE}" >&2
  exit 1
fi

echo "==> Using Python: ${PY_EXE}"
"${PY_EXE}" -c 'import sys; print(f"==> Python version: {sys.version.splitlines()[0]}")'

# 1) Build core static lib if missing
if [ ! -f "${CORE_BUILD}/libLocalMIP.a" ]; then
  echo "==> Core lib not found. Building core (release)..."
  (cd "${REPO_ROOT}" && ./build.sh release)
else
  echo "==> Core lib found at ${CORE_BUILD}/libLocalMIP.a"
fi

# 2) Configure bindings
rm -rf "${BIND_BUILD}"
mkdir -p "${BIND_BUILD}"
echo "==> Configuring bindings..."
PYBIND11_CMAKE_DIR="$("${PY_EXE}" -c 'import pybind11; print(pybind11.get_cmake_dir())' 2>/dev/null || true)"

cmake -S "${BIND_DIR}" -B "${BIND_BUILD}" \
  -DPYTHON_EXECUTABLE="${PY_EXE}" \
  ${PYBIND11_CMAKE_DIR:+-Dpybind11_DIR="${PYBIND11_CMAKE_DIR}"} \
  -Wno-dev

# 3) Build bindings
echo "==> Building bindings..."
cmake --build "${BIND_BUILD}"

echo "==> Done. Module located in:"
ls -1 "${BIND_BUILD}"/localmip_py*.so

echo
echo "To use:"
echo "  ${PY_EXE} python-bindings/sample.py"
echo "  ${PY_EXE} python-bindings/model_api_demo.py"
echo "  ${PY_EXE} python-bindings/test_python_api.py"
