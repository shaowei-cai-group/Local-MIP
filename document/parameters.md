# Local-MIP Parameters

| Parameter | Description | Flag | Type | Range | Default |
|-----------|-------------|------|------|-------|---------|
| `model_file` | Path to the input model file (.mps/.lp). Required. | `-i` | string | - | `""` |
| `sol_path` | Path to the output solution file (.sol). | `-s` | string | - | `""` |
| `param_set_file` | Path to a parameter configuration file (.set). Command-line options override values in this file. | `-c` | string | - | `""` |
| `time_limit` | Time limit in seconds. | `-t` | double | [0, 1e8] | `10` |
| `random_seed` | Random seed for local search (`0` uses the default seed). | `-S` | int | [0, 2147483647] | `0` |
| `feas_tolerance` | Feasibility tolerance. | `-F` | double | [0, 1e-2] | `1e-6` |
| `opt_tolerance` | Optimality tolerance. | `-O` | double | [0, 1] | `1e-4` |
| `zero_tolerance` | Zero value tolerance. | `-Z` | double | [0, 1e-3] | `1e-9` |
| `bound_strengthen` | Bound strengthen level (0-off, 1-ip, 2-mip). | `-b` | int | [0, 2] | `1` |
| `log_obj` | Log objective values during search. | `-l` | boolean | - | `true` |
| `restart_step` | No-improvement steps before restart (0 disables restarts). | `-r` | int | [0, 100000000] | `1000000` |
| `smooth_prob` | Weight smoothing probability, expressed in 1/10000. | `-0` | int | [0, 10000] | `1` |
| `bms_unsat_con` | BMS unsatisfied constraint sample size. | `-u` | int | [0, 100000000] | `12` |
| `bms_unsat_ops` | BMS MTM unsatisfied operations. | `-p` | int | [0, 100000000] | `2250` |
| `bms_sat_con` | BMS satisfied constraint sample size. | `-v` | int | [0, 100000000] | `1` |
| `bms_sat_ops` | BMS MTM satisfied operations. | `-o` | int | [0, 100000000] | `80` |
| `bms_flip_ops` | BMS flip operations. | `-x` | int | [0, 100000000] | `0` |
| `bms_easy_ops` | BMS easy operations. | `-q` | int | [0, 100000000] | `5` |
| `bms_random_ops` | BMS random operations. | `-g` | int | [0, 100000000] | `250` |
| `tabu_base` | Base tabu tenure. | `-a` | int | [0, 100000000] | `4` |
| `tabu_var` | Tabu tenure variation (minimum 1). | `-e` | int | [1, 100000000] | `7` |
| `activity_period` | Constraint activity recompute period. | `-h` | int | [1, 100000000] | `100000` |
| `break_eq_feas` | Break feasibility on equality constraints. | `-z` | boolean | - | `false` |
| `split_eq` | Split equality constraints into two inequalities. | `-j` | boolean | - | `true` |
| `start` | Start method (`zero` or `random`). | `-m` | enum | - | `"zero"` |
| `restart` | Restart strategy (`random`, `best`, or `hybrid`). | `-y` | enum | - | `"best"` |
| `weight` | Constraint weight update method (`smooth` or `monotone`). | `-w` | enum | - | `"monotone"` |
| `lift_scoring` | Feasible-phase lift scoring (`lift_age` or `lift_random`). | `-f` | enum | - | `"lift_age"` |
| `neighbor_scoring` | Infeasible-phase neighbor scoring (`progress_bonus` or `progress_age`). | `-n` | enum | - | `"progress_bonus"` |
