"""
Smoke test for Local-MIP Python bindings.

Covers:
- set_param_set_file on the library API
- structured callback contexts
- optional user_data delivery
- custom neighbor generation and scoring callbacks
"""

import math
import os
import sys
import tempfile
import gc
import weakref

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(THIS_DIR, ".."))
BUILD_DIR = os.path.join(THIS_DIR, "build")
if os.path.isdir(BUILD_DIR):
    sys.path.insert(0, BUILD_DIR)

import localmip_py as lm  # noqa: E402


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_file_solver_smoke():
    model_path = os.path.join(REPO_ROOT, "test-set", "sct1.mps")
    fd, config_path = tempfile.mkstemp(suffix=".set", text=True)
    os.close(fd)
    try:
        with open(config_path, "w", encoding="utf-8") as fp:
            fp.write(f"model_file = {model_path}\n")
            fp.write("time_limit = 0.4\n")
            fp.write("log_obj = 0\n")
            fp.write("bms_unsat_con = 9\n")

        solver = lm.LocalMIP()
        solver.set_param_set_file(config_path)
        solver.set_time_limit(0.8)  # later setters should override the file
        solver.set_log_obj(False)
        solver.set_bms_unsat_con(7)
        solver.set_bms_mtm_unsat_op(33)
        solver.set_bms_sat_con(2)
        solver.set_bms_mtm_sat_op(11)
        solver.set_bms_flip_op(5)
        solver.set_bms_easy_op(9)
        solver.set_bms_random_op(13)
        solver.clear_neighbor_list()

        stats = {
            "start": 0,
            "neighbor": 0,
            "neighbor_score": 0,
            "lift": 0,
            "checked_neighbor_ctx": False,
        }

        def start_cbk(ctx, user_data):
            user_data["start"] += 1
            require(ctx.shared.model_manager.var_num > 0,
                    "shared model_manager should be available in start callback")
            binary_idxs = ctx.shared.binary_idx_list
            require(not isinstance(binary_idxs, list),
                    "shared binary_idx_list should be exposed as a view")
            if binary_idxs:
                var = ctx.shared.model_manager.var(binary_idxs[0])
                require(var.is_binary(), "model var lookup should work in Python")
                ctx.current_values[binary_idxs[0]] = 0.0

        def custom_neighbor(ctx, user_data):
            user_data["neighbor"] += 1
            if not user_data["checked_neighbor_ctx"]:
                require(not isinstance(ctx.shared.var_current_value, list),
                        "shared var_current_value should be exposed as a view")
                try:
                    ctx.op_size = 1
                except AttributeError:
                    pass
                else:
                    raise AssertionError("NeighborCtx.op_size should be read-only")
                user_data["checked_neighbor_ctx"] = True
            binary_idxs = ctx.shared.binary_idx_list
            if not binary_idxs:
                ctx.clear_ops()
                return
            var_idx = binary_idxs[0]
            current_value = ctx.shared.var_current_value[var_idx]
            delta = 1.0 if current_value < 0.5 else -1.0
            require(
                ctx.shared.model_manager.var(var_idx).in_bound(current_value + delta),
                "neighbor callback should be able to inspect bounds",
            )
            ctx.set_single_op(var_idx, delta)

        def neighbor_score_cbk(ctx, var_idx, delta, user_data):
            user_data["neighbor_score"] += 1
            if ctx.best_neighbor_score <= 0:
                ctx.best_var_idx = var_idx
                ctx.best_delta = delta
                ctx.best_neighbor_score = 1
                ctx.best_neighbor_subscore = 0
                ctx.best_age = 0

        def lift_cbk(ctx, var_idx, delta, user_data):
            user_data["lift"] += 1
            require(not isinstance(ctx.shared.var_obj_cost, list),
                    "shared var_obj_cost should be exposed as a view")
            score = -ctx.shared.var_obj_cost[var_idx] * delta
            if ctx.best_var_idx >= ctx.shared.model_manager.var_num or score >= ctx.best_lift_score:
                ctx.best_var_idx = var_idx
                ctx.best_delta = delta
                ctx.best_lift_score = score
                ctx.best_age = 0

        solver.set_start_cbk(start_cbk, stats)
        solver.add_custom_neighbor("py_flip", custom_neighbor, stats)
        solver.set_neighbor_scoring_cbk(neighbor_score_cbk, stats)
        solver.set_lift_scoring_cbk(lift_cbk, stats)

        solver.run()

        require(stats["start"] > 0, "start callback should be invoked")
        require(stats["neighbor"] > 0, "custom neighbor callback should be invoked")
        require(stats["neighbor_score"] > 0,
                "neighbor scoring callback should be invoked")
        require(solver.get_model_manager().var_num > 0,
                "get_model_manager should be exposed")
    finally:
        if os.path.exists(config_path):
            os.remove(config_path)


def run_param_file_error_smoke():
    solver = lm.LocalMIP()

    try:
        solver.set_param_set_file(os.path.join(REPO_ROOT, "missing_params.set"))
    except Exception as exc:
        require("cannot open parameter set file" in str(exc),
                "missing parameter files should raise a descriptive exception")
    else:
        raise AssertionError("missing parameter files should not abort or succeed")

    fd, config_path = tempfile.mkstemp(suffix=".set", text=True)
    os.close(fd)
    try:
        with open(config_path, "w", encoding="utf-8") as fp:
            fp.write("time_limit = nope\n")

        try:
            solver.set_param_set_file(config_path)
        except Exception as exc:
            require("invalid floating value 'nope'" in str(exc),
                    "malformed parameter files should raise a parse exception")
        else:
            raise AssertionError("malformed parameter files should not succeed")
    finally:
        if os.path.exists(config_path):
            os.remove(config_path)

    solver.set_model_file(os.path.join(REPO_ROOT, "test-set", "sct1.mps"))

    fd, config_path = tempfile.mkstemp(suffix=".set", text=True)
    os.close(fd)
    try:
        with open(config_path, "w", encoding="utf-8") as fp:
            fp.write(f"model_file = {os.path.join(REPO_ROOT, 'test-set', '2club200v15p5scn.mps')}\n")
            fp.write("time_limit = 0\n")

        try:
            solver.set_param_set_file(config_path)
        except Exception as exc:
            require("time limit must be positive" in str(exc),
                    "setter-side validation should fail before partial application")
        else:
            raise AssertionError("setter-side validation failures should not succeed")
    finally:
        if os.path.exists(config_path):
            os.remove(config_path)


def run_restart_weight_smoke():
    model_path = os.path.join(REPO_ROOT, "test-set", "2club200v15p5scn.mps")
    solver = lm.LocalMIP()
    solver.set_model_file(model_path)
    solver.set_time_limit(0.3)
    solver.set_log_obj(False)
    solver.set_restart_step(1)
    solver.clear_neighbor_list()

    stats = {
        "restart": 0,
        "weight": 0,
        "noop_neighbor": 0,
    }

    def restart_cbk(ctx, user_data):
        user_data["restart"] += 1
        if ctx.shared.is_found_feasible:
            best_values = ctx.shared.var_best_value
            for idx in range(min(len(best_values), len(ctx.current_values))):
                ctx.current_values[idx] = best_values[idx]

    def weight_cbk(ctx, user_data):
        user_data["weight"] += 1
        if len(ctx.con_weight) > 0:
            ctx.con_weight[0] = max(ctx.con_weight[0], 1)

    def noop_neighbor(ctx, user_data):
        user_data["noop_neighbor"] += 1
        ctx.clear_ops()

    solver.set_restart_cbk(restart_cbk, stats)
    solver.set_weight_cbk(weight_cbk, stats)
    solver.add_custom_neighbor("noop", noop_neighbor, stats)
    solver.run()

    require(stats["noop_neighbor"] > 0, "noop custom neighbor should run")
    require(stats["weight"] > 0, "weight callback should run")
    require(stats["restart"] > 0, "restart callback should run")


def run_callback_lifetime_and_bool_view_smoke():
    solver = lm.LocalMIP()
    solver.enable_model_api()
    solver.set_sense(lm.Sense.minimize)
    solver.set_time_limit(0.1)
    solver.set_log_obj(False)

    x = solver.add_var("x", 0.0, 1.0, 1.0, lm.VarType.binary)
    y = solver.add_var("y", 0.0, 1.0, 0.0, lm.VarType.binary)
    solver.add_con(1.0, 1.0, [x, y], [1.0, 1.0])

    captured = {"ctx": None, "start": 0, "shared_flags": None, "shared_iter_flags": None}

    def start_cbk(ctx, user_data):
        require(user_data is None,
                "explicit None user_data should be forwarded to Python callbacks")
        captured["start"] += 1
        captured["ctx"] = ctx
        captured["shared_flags"] = ctx.shared.con_is_equality.to_list()
        captured["shared_iter_flags"] = list(ctx.shared.con_is_equality)

    solver.set_start_cbk(start_cbk, None)
    solver.run()

    require(captured["start"] > 0, "start callback should run for callback lifetime test")
    manager_flags = solver.get_model_manager().con_is_equality.to_list()
    require(manager_flags == list(solver.get_model_manager().con_is_equality),
            "ModelManager.con_is_equality should support both to_list() and iteration")
    require(all(isinstance(flag, bool) for flag in manager_flags),
            "ModelManager.con_is_equality should expose bool values")
    require(captured["shared_flags"] == captured["shared_iter_flags"],
            "ReadonlyCtx.con_is_equality should support both to_list() and iteration")
    require(all(isinstance(flag, bool) for flag in captured["shared_flags"]),
            "ReadonlyCtx.con_is_equality should expose bool values")
    require(captured["shared_flags"] == manager_flags,
            "ReadonlyCtx.con_is_equality should match ModelManager.con_is_equality")

    solver_ref = weakref.ref(solver)
    del solver
    gc.collect()

    require(solver_ref() is not None,
            "cached callback contexts should keep the solver alive")
    require(captured["ctx"].shared.model_manager.var_num == 2,
            "cached callback contexts should remain valid after solver deletion")
    require(captured["ctx"].shared.con_is_equality.to_list() == manager_flags,
            "cached callback views should remain valid after solver deletion")

    del captured["ctx"]
    gc.collect()
    require(solver_ref() is None,
            "solver should be collectable once cached callback contexts are released")


def run_model_api_smoke():
    solver = lm.LocalMIP()
    solver.enable_model_api()
    solver.set_sense(lm.Sense.maximize)
    legacy_calls = {"start": 0}

    def legacy_start(ctx):
        legacy_calls["start"] += 1

    solver.set_start_cbk(legacy_start)

    x = solver.add_var("x", 0.0, 1.0, 1.0, lm.VarType.binary)
    y = solver.add_var("y", 0.0, 2.0, 0.5, lm.VarType.general_integer)

    manager = solver.get_model_manager()
    require(manager.var_num == 0,
            "model manager should remain empty before the programmatic model is materialized")
    try:
        manager.var(0)
    except IndexError:
        pass
    else:
        raise AssertionError("ModelManager.var should raise IndexError on invalid access")
    try:
        manager.obj()
    except ValueError:
        pass
    else:
        raise AssertionError("ModelManager.obj should raise ValueError when unavailable")
    try:
        manager.con_idx("missing")
    except KeyError:
        pass
    else:
        raise AssertionError("ModelManager.con_idx should raise KeyError for unknown names")

    solver.add_con(-math.inf, 2.0, [x, y], [1.0, 1.0])
    solver.set_time_limit(0.1)
    solver.set_log_obj(False)
    solver.run()

    require(solver.get_model_manager().is_min is False,
            "ModelManager.is_min should be a real boolean for maximize models")
    sol = solver.get_solution()
    require(len(sol) == 2, "model API solution vector should be exposed")
    require(legacy_calls["start"] > 0,
            "legacy no-user_data callback signatures should still work")

    cost_view = solver.get_model_manager().var_obj_cost
    require(len(cost_view) == 2, "view should expose model-manager vector contents")
    expected_costs = cost_view.to_list()
    del solver
    gc.collect()
    require(cost_view.to_list() == expected_costs,
            "vector views should remain valid while they keep the owner alive")


def main():
    run_param_file_error_smoke()
    run_file_solver_smoke()
    run_restart_weight_smoke()
    run_callback_lifetime_and_bool_view_smoke()
    run_model_api_smoke()
    print("Python binding smoke tests passed.")


if __name__ == "__main__":
    main()
