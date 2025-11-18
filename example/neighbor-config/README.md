# Neighbor Config Example

This example shows how to customize the neighbor list, order, and custom neighbor operations.

## What It Does

- Reset or reorder the default neighbor list
- Add predefined neighbors with explicit BMS parameters
- Add custom neighbors with `void* user_data`
- Mix predefined and custom neighbors in any order

## Key APIs

- `clear_neighbor_list()` — remove all default neighbors
- `add_neighbor(name, bms_con, bms_op)` — add a built-in neighbor with BMS counts
- `add_custom_neighbor(name, func, user_data)` — add a user-defined neighbor
- `reset_default_neighbor_list()` — restore the built-in list

Custom neighbors in this example:

1. **my_random_flip_neighbor** — randomly flip one binary variable
2. **my_gradient_descent_neighbor** — gradient-style step to improve the objective (feasible phase only)

## Build & Run

Build all examples from project root:

```bash
cd example
./build.sh
```

Build/run this example only (requires main project built to provide `build/libLocalMIP.a`):

```bash
cd example/neighbor-config
g++ -O3 -std=c++20 neighbor_config.cpp -I../../src -L../../build -lLocalMIP -lpthread -o neighbor_config_demo
cd ..  # run from example/ so test-set/ is in place
./neighbor-config/neighbor_config_demo ./test-set/2club200v15p5scn.mps   # or another .mps file
```

## Sample Output (excerpt)

```
========== Example 1: Use default Neighbor list ==========
c Use default list
...
========== Example 5: Mix predefined and custom Neighbor ==========
c List: [my_random_flip, unsat_mtm_bm, flip, my_gradient_descent]
...
```

## Notes

- Custom neighbors must set `m_op_size` and fill `m_op_var_idxs[]` / `m_op_var_deltas[]`; for no-op set `m_op_size = 0`.
- `add_neighbor` parameters `bms_con` and `bms_op` control BMS candidate and operation counts.
- Call `reset_default_neighbor_list()` to restore the default configuration.
