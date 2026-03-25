import math

import localmip_py as lm


def main():
    solver = lm.LocalMIP()
    solver.enable_model_api()
    solver.set_sense(lm.Sense.maximize)
    solver.set_time_limit(0.1)
    solver.set_log_obj(False)

    x1 = solver.add_var("x1", 0.0, 10.0, 1.0, lm.VarType.real)
    x2 = solver.add_var("x2", 0.0, 10.0, 2.0, lm.VarType.real)
    solver.add_con(-math.inf, 8.0, [x1, x2], [1.0, 1.0])

    solver.run()
    if not solver.is_feasible():
        raise RuntimeError("Local-MIP smoke test did not find a feasible solution")


if __name__ == "__main__":
    main()
