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

    stats = {"calls": 0}

    # Optional: structured start callback with Python-side state.
    def start_cbk(ctx, user_data):
        user_data["calls"] += 1
        binary_idxs = ctx.shared.binary_idx_list
        if not binary_idxs:
            return
        pick = ctx.rng.randint(0, len(binary_idxs) - 1)
        ctx.current_values[binary_idxs[pick]] = 1.0

    solver.set_start_cbk(start_cbk, stats)

    solver.run()
    print("Feasible:", solver.is_feasible())
    print("Start callback calls:", stats["calls"])
    if solver.is_feasible():
        print("Objective:", solver.get_obj_value())
        print("Solution written to py_example.sol")


if __name__ == "__main__":
    main()
