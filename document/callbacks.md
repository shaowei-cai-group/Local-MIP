# Local-MIP Callbacks and Contexts

Local-MIP exposes several callback hooks so you can customize starts, restarts, weights, neighbors, and scoring. All callback contexts share a `Readonly_Ctx` view that gives read-only access to the current search state.

## Common Read-only Context (`Readonly_Ctx`)
Available in every callback via `ctx.m_shared`:
- `m_model_manager` — model metadata (variables, constraints, bounds, coefficients)
- `m_var_current_value` — current variable values
- `m_var_best_value` — best solution found so far
- `m_con_activity` / `m_con_constant` / `m_con_is_equality` — constraint activities, RHS, equality flags
- `m_con_weight` — constraint weights (index 0 is the objective)
- `m_con_unsat_idxs` / `m_con_pos_in_unsat_idxs` / `m_con_sat_idxs` — unsatisfied/satisfied constraint indices and positions
- `m_var_last_*` / `m_var_allow_*` — last improve/decrease steps and allowed steps for variables
- `m_obj_var_num`, `m_var_obj_cost` — objective dimension and coefficients
- `m_is_found_feasible`, `m_best_obj`, `m_current_obj_breakthrough`
- `m_cur_step`, `m_last_improve_step`
- `m_binary_idx_list`, `m_non_fixed_var_idx_list`

## Start Callback
Signature: `void callback(Start::Start_Ctx& ctx, void* user_data)`

Purpose: initialize variable values before search.

Key writable fields:
- `ctx.m_var_current_value` — set the starting assignment.
- `ctx.m_rng` — RNG for reproducible initialization.

Predefined example: `example/start-callback` shows random initialization of binaries with a call counter.

## Restart Callback
Signature: `void callback(Restart::Restart_Ctx& ctx, void* user_data)`

Purpose: control actions at restart (e.g., perturb solutions, reset weights).

Key writable fields:
- `ctx.m_var_current_value` — current assignment you can perturb.
- `ctx.m_con_weight` — constraint weights (index 0 = objective).
- `ctx.m_rng` — RNG.

Predefined example: `example/restart-callback` flips a subset of binaries and resets weights.

## Weight Callback
Signature: `void callback(Weight::Weight_Ctx& ctx, void* user_data)`

Purpose: customize constraint weight updates.

Key writable fields:
- `ctx.m_con_weight` — adjust weights (0 = objective).
- `ctx.m_rng` — RNG.

Predefined example: `example/weight-callback` increases weights for violated constraints and objective when feasible.

## Neighbor Callback (custom neighbor generator)
Signature: `void callback(Neighbor::Neighbor_Ctx& ctx, void* user_data)`

Purpose: generate neighbor moves.

Key writable fields:
- `ctx.m_op_size`, `ctx.m_op_var_idxs[]`, `ctx.m_op_var_deltas[]` — define the neighbor move.
- `ctx.m_rng` — RNG.

Predefined example: `example/neighbor-config` mixes built-in neighbors with custom ones; `example/neighbor-userdata` demonstrates user data.

## Neighbor Scoring Callback (infeasible phase)
Signature: `void callback(Scoring::Neighbor_Ctx& ctx, size_t var_idx, double delta, void* user_data)`

Purpose: rank neighbor candidates while seeking feasibility.

Key writable fields:
- `ctx.m_best_var_idx`, `ctx.m_best_delta`, `ctx.m_best_neighbor_score`, `ctx.m_best_neighbor_subscore` — update best candidate tracking.

Predefined example: `example/scoring-neighbor` implements a multi-level tie-breaker with bonus scores.

## Lift Scoring Callback (feasible phase)
Signature: `void callback(Scoring::Lift_Ctx& ctx, size_t var_idx, double delta, void* user_data)`

Purpose: score candidate moves in the feasible optimization phase.

Key writable fields:
- `ctx.m_best_var_idx`, `ctx.m_best_delta`, `ctx.m_best_lift_score`, `ctx.m_best_age`.

Predefined example: `example/scoring-lift` uses variable degree as a tie-breaker.

## How to Register
- C++ API exposes setters such as `set_start_cbk`, `set_restart_cbk`, `set_weight_cbk`, `set_neighbor_cbk`, `set_neighbor_scoring_cbk`, `set_lift_scoring_cbk`.
- Python bindings expose analogous registration functions with opaque context capsules; extend the binding for field-level access.

## Tips
- Use `user_data` to pass custom state across calls.
- `ctx.m_shared` is read-only; mutate only the writable fields listed for each callback type.
- Run from `example/` after `prepare.sh` to reuse the shipped test instances when trying demos.
