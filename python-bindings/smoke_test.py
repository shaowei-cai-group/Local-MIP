import math
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(THIS_DIR, "build")
if os.path.isdir(BUILD_DIR):
    sys.path.insert(0, BUILD_DIR)

import localmip_py as lm


def main():
    builder = lm.ModelBuilder()
    builder.set_sense(lm.Sense.maximize)

    x1 = builder.add_var("x1", 0.0, 10.0, 1.0, lm.VarType.real)
    x2 = builder.add_var("x2", 0.0, 10.0, 2.0, lm.VarType.real)
    builder.add_con(-math.inf, 8.0, [x1, x2], [1.0, 1.0])

    solver = lm.LocalMIP(builder.prepare())
    solver.set_time_limit(0.1)
    solver.set_log_obj(False)
    solver.run()
    if not solver.is_feasible():
        raise RuntimeError("Local-MIP smoke test did not find a feasible solution")


if __name__ == "__main__":
    main()
