# Terminology (theory-facing)

## Mixed Integer Programming (MIP)
- Canonical minimization form with mixed domains:

  $$
  \begin{aligned}
  \min_{x} \quad & c^\top x \\
  \text{s.t.} \quad
  & A^{(\le)} x \le b^{(\le)}, \\
  & A^{(=)} x = b^{(=)}, \\
  & \ell \le x \le u, \\
  & x_i \in \mathbb{Z} \quad \forall i \in I, \\
  & x_i \in \{0,1\} \quad \forall i \in B, \\
  & x_j \in \mathbb{R} \quad \forall j \notin I \cup B,
  \end{aligned}
  $$

  where \(I\) is the index set of general integers, \(B\) is the index set of binaries, and other variables are continuous.
- Core notions: objective coefficients \(c\), constraint matrices \(A^{(\le)}, A^{(=)}\), bounds \(\ell, u\), feasibility (constraint satisfaction), optimality (no better feasible objective).

## Local Search (LS) for MIP
- State: a numerical assignment \(x\) (not necessarily feasible) plus a record of feasibility status.
- Move (delta): a modification \((i, \Delta)\) applied to a variable, potentially bundled as a multi-variable operation.
- Neighborhood: the set of candidate moves considered from the current state; can mix predefined and user-defined generators.
- Iteration/step: the discrete progression index of the search; stagnation is measured by steps since the last improvement.
- Constraint violation: degree to which constraints are broken; violation weights bias attention toward specific constraints.
- Scores vs. objective: when infeasible, weighted violation scores guide search; when feasible, the true objective drives improvement.

## Core search components in Local-MIP
- Best-from-Multiple-Selection (BMS): sample a small set of constraints/moves and take the best candidate to balance exploration and exploitation.
- Tabu memory: short-term prohibition of reversing recent moves to prevent cycling; parameterized by a base tenure and variation.
- Restarts: periodic perturbation of the search state to escape stagnation.
- Starts: strategies for building an initial assignment (e.g., zero, random, heuristic).
- Weight updates: schemes that adapt constraint weights over time to emphasize difficult or newly violated constraints.

## Scoring and selection
- Infeasible phase: neighbor scoring ranks moves by their expected reduction in constraint violation (optionally with bonuses for breakthroughs).
- Feasible phase: lift scoring ranks moves by projected objective improvement.
- Tie-breaking: multi-level comparison (primary score, secondary bonus/subscore, possible randomness) to diversify choices.

## Callback interfaces (conceptual)
- Start callback: user-defined initialization before the search begins.
- Restart callback: actions applied when a restart is triggered (e.g., perturb assignments, reset or rescale weights).
- Weight callback: custom policies for updating constraint weights during the search.
- Neighbor generation callback: construct candidate moves from the current state.
- Neighbor scoring callback: prioritize candidates in the infeasible phase.
- Lift scoring callback: prioritize candidates in the feasible phase.
- Shared read-only context: a consistent view of model data, current state, best-known solution, and violation/objective metrics exposed to callbacks without modification rights.

## Files and artifacts
- Model files: `.mps/.lp` instances defining variables, bounds, objectives, and constraints.
- Solution files: `.sol` outputs containing variable assignments.
- Parameter sets: `.set` configuration files describing solver options (command-line overrides apply).
