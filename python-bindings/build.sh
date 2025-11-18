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
PY_EXE="${PYTHON_EXECUTABLE:-$(python3 -c 'import sys;print(sys.executable)')}"

echo "==> Using Python: ${PY_EXE}"

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
cmake -S "${BIND_DIR}" -B "${BIND_BUILD}" \
  -DPython_EXECUTABLE="${PY_EXE}" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# 3) Build bindings
echo "==> Building bindings..."
cmake --build "${BIND_BUILD}"

echo "==> Done. Module located in:"
ls -1 "${BIND_BUILD}"/localmip_py*.so

echo
echo "To use:"
echo "  export PYTHONPATH=${BIND_BUILD}:\${PYTHONPATH}"
echo "  python3 python-bindings/sample.py"
