"""
Python demo for Local-MIP Model API (programmatic model building).
This mirrors example/model-api/model_api_demo.cpp.

Build the module first:
  PY_EXE="$(python3 -c 'import sys; print(sys.executable)')"
  PYTHON_EXECUTABLE="${PY_EXE}" python-bindings/build.sh

Run:
  "${PY_EXE}" python-bindings/model_api_demo.py
"""

import math
import os
import sys

this_dir = os.path.dirname(os.path.abspath(__file__))
build_dir = os.path.join(this_dir, "build")
if os.path.isdir(build_dir):
    sys.path.insert(0, build_dir)

import localmip_py as lm  # noqa: E402


def main():
    inf = math.inf

    builder = lm.ModelBuilder()
    builder.set_sense(lm.Sense.maximize)

    print("Building model...")

    x1 = builder.add_var("x1", 0.0, 40.0, 1.0, lm.VarType.real)
    x2 = builder.add_var("x2", 0.0, inf, 2.0, lm.VarType.real)
    x3 = builder.add_var("x3", 0.0, inf, 3.0, lm.VarType.real)
    x4 = builder.add_var(
        "x4", 2.0, 3.0, 1.0, lm.VarType.general_integer)

    print("Added 4 variables: x1, x2, x3, x4")

    builder.add_con(
        -inf, 20.0, [x1, x2, x3, x4], [-1.0, 1.0, 1.0, 10.0])
    builder.add_con(-inf, 30.0, [x1, x2, x3], [1.0, -3.0, 1.0])
    builder.add_con(0.0, 0.0, [x2, x4], [1.0, -3.5])

    print("Added 3 constraints")

    model = builder.prepare()
    solver = lm.LocalMIP(model)
    solver.set_time_limit(1.0)
    solver.set_log_obj(True)

    print("\nStarting solver...")
    print("=====================================")
    solver.run()
    print("=====================================")

    print("\nResults:")
    print("  Objective value:", solver.get_obj_value())
    print("  Feasible:", "Yes" if solver.is_feasible() else "No")

    if solver.is_feasible():
        sol = solver.get_solution()
        print("  Solution:")
        print("    x1 =", sol[0])
        print("    x2 =", sol[1])
        print("    x3 =", sol[2])
        print("    x4 =", sol[3])


if __name__ == "__main__":
    main()
