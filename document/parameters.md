# Local-MIP Parameters

`model_file`
Path to the input model file (.mps/.lp). Required.
Flag: `-i`
Type: string
Default: ""

`sol_path`
Path to the output solution file (.sol).
Flag: `-s`
Type: string
Default: ""

`param_set_file`
Path to a parameter configuration file (.set). Command-line options override values in this file.
Flag: `-c`
Type: string
Default: ""

`time_limit`
Time limit in seconds.
Flag: `-t`
Type: double
Range: [0, 1e8]
Default: 10

`random_seed`
Random seed for local search (`0` uses the default seed).
Flag: `-S`
Type: int
Range: [0, 2147483647]
Default: 0

`feas_tolerance`
Feasibility tolerance.
Flag: `-F`
Type: double
Range: [0, 1e-2]
Default: 1e-6

`opt_tolerance`
Optimality tolerance.
Flag: `-O`
Type: double
Range: [0, 1]
Default: 1e-4

`zero_tolerance`
Zero value tolerance.
Flag: `-Z`
Type: double
Range: [0, 1e-3]
Default: 1e-9

`bound_strengthen`
Bound strengthen level (0-off, 1-ip, 2-mip).
Flag: `-b`
Type: int
Range: [0, 2]
Default: 1

`log_obj`
Log objective values during search.
Flag: `-l`
Type: boolean
Default: true

`restart_step`
No-improvement steps before restart (0 disables restarts).
Flag: `-r`
Type: int
Range: [0, 100000000]
Default: 1000000

`smooth_prob`
Weight smoothing probability, expressed in 1/10000.
Flag: `-0`
Type: int
Range: [0, 10000]
Default: 1

`bms_unsat_con`
BMS unsatisfied constraint sample size.
Flag: `-u`
Type: int
Range: [0, 100000000]
Default: 12

`bms_unsat_ops`
BMS MTM unsatisfied operations.
Flag: `-p`
Type: int
Range: [0, 100000000]
Default: 2250

`bms_sat_con`
BMS satisfied constraint sample size.
Flag: `-v`
Type: int
Range: [0, 100000000]
Default: 1

`bms_sat_ops`
BMS MTM satisfied operations.
Flag: `-o`
Type: int
Range: [0, 100000000]
Default: 80

`bms_flip_ops`
BMS flip operations.
Flag: `-x`
Type: int
Range: [0, 100000000]
Default: 0

`bms_easy_ops`
BMS easy operations.
Flag: `-q`
Type: int
Range: [0, 100000000]
Default: 5

`bms_random_ops`
BMS random operations.
Flag: `-g`
Type: int
Range: [0, 100000000]
Default: 250

`tabu_base`
Base tabu tenure.
Flag: `-a`
Type: int
Range: [0, 100000000]
Default: 4

`tabu_var`
Tabu tenure variation (minimum 1).
Flag: `-e`
Type: int
Range: [1, 100000000]
Default: 7

`activity_period`
Constraint activity recompute period.
Flag: `-h`
Type: int
Range: [1, 100000000]
Default: 100000

`break_eq_feas`
Break feasibility on equality constraints.
Flag: `-z`
Type: boolean
Default: false

`split_eq`
Split equality constraints into two inequalities.
Flag: `-j`
Type: boolean
Default: true

`start`
Start method (`zero` or `random`).
Flag: `-m`
Type: enum
Default: "zero"

`restart`
Restart strategy (`random`, `best`, or `hybrid`).
Flag: `-y`
Type: enum
Default: "best"

`weight`
Constraint weight update method (`smooth` or `monotone`).
Flag: `-w`
Type: enum
Default: "monotone"

`lift_scoring`
Feasible-phase lift scoring (`lift_age` or `lift_random`).
Flag: `-f`
Type: enum
Default: "lift_age"

`neighbor_scoring`
Infeasible-phase neighbor scoring (`progress_bonus` or `progress_age`).
Flag: `-n`
Type: enum
Default: "progress_bonus"
