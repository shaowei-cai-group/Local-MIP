# Python Bindings (pybind11)

This folder provides a pybind11 module that exposes `Local_MIP` to Python without touching the main solver code.

The bindings expose the core solver configuration and result-query API used in normal runs, including `.set` parameter-file loading, plus the in-memory modeling interface.

## Prerequisites
- Build the solver first (produces `build/libLocalMIP.a`):
  ```bash
  ./build.sh release   # or debug
  # or build everything in one go:
  PYTHON_EXECUTABLE="$(python3 -c 'import sys; print(sys.executable)')" ./build.sh all
  ```
- Install pybind11 (system package or `pip install pybind11`).

## Build the Python module
From repo root:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
cmake -S python-bindings -B python-bindings/build \
  -DPYTHON_EXECUTABLE="${PY_EXE}" \
  -Wno-dev
cmake --build python-bindings/build
```
Output: `python-bindings/build/localmip_py.cpython-*.so` (exact suffix depends on Python version).

Alternatively, run the helper script (builds the core if missing):
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
PYTHON_EXECUTABLE="${PY_EXE}" python-bindings/build.sh
```

Use the same interpreter for build and run so the compiled module suffix matches the interpreter that imports it:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
PYTHON_EXECUTABLE="${PY_EXE}" python-bindings/build.sh
"${PY_EXE}" python-bindings/sample.py
"${PY_EXE}" python-bindings/model_api_demo.py
"${PY_EXE}" python-bindings/test_python_api.py
```

## Use in Python
Either append the build directory to `PYTHONPATH` or copy the `.so` next to your script:
```python
import sys
sys.path.append("python-bindings/build")  # if not installed
import localmip_py as lm

solver = lm.LocalMIP()
solver.set_model_file("test-set/sct1.mps")
solver.set_sol_path("py_example.sol")
solver.set_time_limit(10.0)
solver.set_log_obj(True)

# Optional: register a start callback
stats = {"calls": 0}

def start_cbk(ctx, user_data):
    user_data["calls"] += 1
    if ctx.shared.binary_idx_list:
        idx = ctx.shared.binary_idx_list[0]
        ctx.current_values[idx] = 1.0

solver.set_start_cbk(start_cbk, stats)
solver.run()

print("Feasible:", solver.is_feasible(), "Obj:", solver.get_obj_value())
print("Start callback calls:", stats["calls"])
```

## Model API (Programmatic Modeling)
The bindings also expose Local-MIP's **Model API**, which lets you build a model
directly in Python (mirrors `example/model-api/model_api_demo.cpp`).

Run the demo:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
"${PY_EXE}" python-bindings/model_api_demo.py
```

Smoke test:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
"${PY_EXE}" python-bindings/test_python_api.py
```

Key symbols:
- `lm.Sense.{minimize,maximize}`
- `lm.VarType.{binary,general_integer,real,fixed}`
- `LocalMIP.set_param_set_file(...)`
- `LocalMIP.set_bms_unsat_con(...)`, `set_bms_mtm_unsat_op(...)`, `set_bms_sat_con(...)`, `set_bms_mtm_sat_op(...)`, `set_bms_flip_op(...)`, `set_bms_easy_op(...)`, `set_bms_random_op(...)`
- `LocalMIP.enable_model_api()`
- `LocalMIP.add_var(...)`, `LocalMIP.add_con(...)`
- `LocalMIP.get_solution()`

## Notes
- The module links against the static library at `build/libLocalMIP.a`; re-run `./build.sh` if the core changes.
- Long-running `run()` releases the GIL; Python callbacks reacquire the GIL before execution.
- Callback contexts (`Start::Start_Ctx`, etc.) are exposed as structured Python objects with read-only shared sequence views and writable fields where the solver expects mutation.
- `NeighborCtx.op_size` is derived from `clear_ops()`, `set_single_op(...)`, and `append_op(...)`; update move outputs through those helpers instead of writing the size directly.
- Python callbacks can optionally receive a Python `user_data` object; if omitted, the old shorter callback signatures still work.
