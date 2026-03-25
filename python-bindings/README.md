# Python Bindings (pybind11)

This folder provides a pybind11 module that exposes `Local_MIP` to Python without touching the main solver code.

The bindings expose the core solver configuration and result-query API used in normal runs, including `.set` parameter-file loading, plus the in-memory modeling interface.

## Install from PyPI
For Linux x86_64:
```bash
python3 -m pip install localmip
```

Import it directly:
```python
import localmip_py as lm
```

## Install from the repository
From repo root:
```bash
python3 -m pip install ./python-bindings
```

This builds both the Local-MIP core and the pybind11 extension during installation.
After installation, the module can be imported directly:
```bash
python3 -c "import localmip_py as lm; print(lm.LocalMIP)"
```

## Run the demos
From repo root:
```bash
python3 python-bindings/sample.py
python3 python-bindings/model_api_demo.py
python3 python-bindings/test_python_api.py
python3 python-bindings/smoke_test.py
```

## Development build without pip
The legacy local build flow is still supported. From repo root:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
PYTHON_EXECUTABLE="${PY_EXE}" python-bindings/build.sh
```

Output: `python-bindings/build/localmip_py.cpython-*.so` (exact suffix depends on Python version).

Use the same interpreter for build and run so the compiled module suffix matches the interpreter that imports it:
```bash
PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
PYTHON_EXECUTABLE="${PY_EXE}" python-bindings/build.sh
"${PY_EXE}" python-bindings/sample.py
"${PY_EXE}" python-bindings/model_api_demo.py
"${PY_EXE}" python-bindings/test_python_api.py
"${PY_EXE}" python-bindings/smoke_test.py
```

## Use in Python
If installed with pip:
```python
import localmip_py as lm
```

If using the local development build, either put the build directory at the front of `PYTHONPATH` or copy the `.so` next to your script:
```python
import sys
sys.path.insert(0, "python-bindings/build")  # prefer the local build
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
- `pip install ./python-bindings` compiles the Local-MIP core and the extension together inside the wheel build.
- The local helper script `python-bindings/build.sh` still works for iterative development builds.
- Long-running `run()` releases the GIL; Python callbacks reacquire the GIL before execution.
- Callback contexts (`Start::Start_Ctx`, etc.) are exposed as structured Python objects with read-only shared sequence views and writable fields where the solver expects mutation.
- `NeighborCtx.op_size` is derived from `clear_ops()`, `set_single_op(...)`, and `append_op(...)`; update move outputs through those helpers instead of writing the size directly.
- Python callbacks can optionally receive a Python `user_data` object; if omitted, the old shorter callback signatures still work.
