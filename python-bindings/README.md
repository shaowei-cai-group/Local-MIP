# Python Bindings (pybind11)

This folder provides a minimal pybind11 module that exposes `Local_MIP` to Python without touching the main solver code.

## Prerequisites
- Build the solver first (produces `build/libLocalMIP.a`):
  ```bash
  ./build.sh release   # or debug
  ```
- Install pybind11 (system package or `pip install pybind11`).

## Build the Python module
From repo root:
```bash
cmake -S python-bindings -B python-bindings/build \
  -DPython_EXECUTABLE="$(python3 -c 'import sys;print(sys.executable)')" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build python-bindings/build
```
Output: `python-bindings/build/localmip_py.cpython-*.so` (exact suffix depends on Python version).

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
def start_cbk(ctx_capsule):
    # ctx_capsule is an opaque pointer to Start::Start_Ctx (use pybind11 extension if you need fields)
    pass

solver.set_start_cbk(start_cbk)
solver.run()

print("Feasible:", solver.is_feasible(), "Obj:", solver.get_obj_value())
```

## Notes
- The module links against the static library at `build/libLocalMIP.a`; re-run `./build.sh` if the core changes.
- Long-running `run()` releases the GIL; Python callbacks reacquire the GIL before execution.
- Callback contexts (`Start::Start_Ctx`, etc.) are passed as opaque pointers; extend the binding if you need to inspect fields in Python.
