"""
Minimal Python example calling Local-MIP via pybind11 bindings.
Requires the built module (see build.sh) and a test instance in test-set/.
"""
import os
import sys

# Point to the built module if not installed
this_dir = os.path.dirname(os.path.abspath(__file__))
bind_build = os.path.join(this_dir, "build")
sys.path.append(bind_build)

import localmip_py as lm


def main():
    solver = lm.LocalMIP()
    solver.set_model_file(os.path.join(this_dir, "..", "test-set", "2club200v15p5scn.mps"))
    solver.set_sol_path("py_example.sol")
    solver.set_time_limit(5.0)
    solver.set_log_obj(True)

    # Optional: simple start callback (no-op)
    def start_cbk(ctx_capsule):
        # ctx_capsule is an opaque pointer; extend bindings if you need fields.
        return

    solver.set_start_cbk(start_cbk)

    solver.run()
    print("Feasible:", solver.is_feasible())
    if solver.is_feasible():
        print("Objective:", solver.get_obj_value())
        print("Solution written to py_example.sol")


if __name__ == "__main__":
    main()
