"""
Python demo for Local-MIP Model API (programmatic model building).
This mirrors example/model-api/model_api_demo.cpp.

Build the module first:
  cmake -S python-bindings -B python-bindings/build \
    -DPYTHON_EXECUTABLE="$(python3 -c 'import sys;print(sys.executable)')" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -Wno-dev
  cmake --build python-bindings/build

Run:
  python3 python-bindings/model_api_demo.py
"""

import math
import os
import sys

this_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(this_dir, "build"))

import localmip_py as lm  # noqa: E402


def main():
    inf = math.inf

    solver = lm.LocalMIP()
    solver.enable_model_api()
    solver.set_sense(lm.Sense.maximize)

    solver.set_time_limit(1.0)
    solver.set_log_obj(True)

    print("Building model...")

    x1 = solver.add_var("x1", 0.0, 40.0, 1.0, lm.VarType.real)
    x2 = solver.add_var("x2", 0.0, inf, 2.0, lm.VarType.real)
    x3 = solver.add_var("x3", 0.0, inf, 3.0, lm.VarType.real)
    x4 = solver.add_var("x4", 2.0, 3.0, 1.0, lm.VarType.general_integer)

    print("Added 4 variables: x1, x2, x3, x4")

    solver.add_con(-inf, 20.0, [x1, x2, x3, x4], [-1.0, 1.0, 1.0, 10.0])
    solver.add_con(-inf, 30.0, [x1, x2, x3], [1.0, -3.0, 1.0])
    solver.add_con(0.0, 0.0, [x2, x4], [1.0, -3.5])

    print("Added 3 constraints")

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
