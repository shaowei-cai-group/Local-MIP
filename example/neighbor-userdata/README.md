# Neighbor User Data Example

Demonstrates how to pass custom data structures to custom neighbor operations using `void* p_user_data`.

## What It Does

This example shows how to:
- Define custom neighbor operations (not just callbacks)
- Pass user data structures to custom neighbors
- Track statistics across multiple custom operations
- Mix custom and predefined neighbors
- Use `add_custom_neighbor()` API

## Why Use Custom Neighbors?

Custom neighbor operations allow:
- Implement domain-specific move operations
- Combine multiple variables in a single move
- Use problem structure for guided search
- Share state/statistics across operations

## Custom Neighbor vs Neighbor Scoring

| Aspect | Custom Neighbor | Neighbor Scoring Callback |
|--------|-----------------|---------------------------|
| **Purpose** | Define complete move operations | Score predefined moves |
| **Control** | Full control over move generation | Control over move selection |
| **Output** | Variable indices + deltas | Update best variable/score |
| **Flexibility** | Can create any move | Limited to scoring logic |
| **Typical Use** | Problem-specific operators | Tie-breaking strategies |

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

// User-defined statistics structure
struct NeighborStats
{
  size_t total_calls = 0;
  size_t successful_ops = 0;
  size_t failed_ops = 0;
  size_t binary_flips = 0;
  size_t gradient_steps = 0;
};

// Custom Neighbor 1: Smart binary variable flip
void smart_flip_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  NeighborStats* stats = static_cast<NeighborStats*>(p_user_data);
  if (stats) stats->total_calls++;

  // Find binary variable with best objective improvement
  const auto& binary_list = ctx.m_shared.m_binary_idx_list;
  size_t best_var_idx = SIZE_MAX;
  double best_improvement = 0.0;

  for (size_t var_idx : binary_list)
  {
    double current_value = ctx.m_shared.m_var_current_value[var_idx];
    double obj_cost = ctx.m_shared.m_var_obj_cost[var_idx];
    double delta = (current_value < 0.5) ? 1.0 : -1.0;
    double improvement = -delta * obj_cost;

    if (improvement > best_improvement)
    {
      best_improvement = improvement;
      best_var_idx = var_idx;
    }
  }

  // Set operation: flip best variable
  if (best_var_idx != SIZE_MAX)
  {
    ctx.m_op_size = 1;
    ctx.m_op_var_idxs[0] = best_var_idx;
    ctx.m_op_var_deltas[0] = (ctx.m_shared.m_var_current_value[best_var_idx] < 0.5) ? 1.0 : -1.0;

    if (stats)
    {
      stats->successful_ops++;
      stats->binary_flips++;
    }
  }
}

// Custom Neighbor 2: Greedy gradient descent
void greedy_gradient_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  NeighborStats* stats = static_cast<NeighborStats*>(p_user_data);
  if (stats) stats->total_calls++;

  // Only operate in feasible phase
  if (!ctx.m_shared.m_is_found_feasible)
  {
    ctx.m_op_size = 0;
    if (stats) stats->failed_ops++;
    return;
  }

  // Find variable with best gradient direction
  size_t best_var_idx = SIZE_MAX;
  double best_delta = 0.0;
  double best_improvement = 0.0;

  for (size_t var_idx : ctx.m_shared.m_non_fixed_var_idx_list)
  {
    double obj_cost = ctx.m_shared.m_var_obj_cost[var_idx];
    const auto& var = ctx.m_shared.m_model_manager.var(var_idx);
    double current = ctx.m_shared.m_var_current_value[var_idx];

    // Try increase (if cost negative)
    if (obj_cost < 0 && current < var.upper_bound())
    {
      double improvement = -obj_cost * 1.0;
      if (improvement > best_improvement)
      {
        best_improvement = improvement;
        best_var_idx = var_idx;
        best_delta = 1.0;
      }
    }

    // Try decrease (if cost positive)
    if (obj_cost > 0 && current > var.lower_bound())
    {
      double improvement = -obj_cost * (-1.0);
      if (improvement > best_improvement)
      {
        best_improvement = improvement;
        best_var_idx = var_idx;
        best_delta = -1.0;
      }
    }
  }

  // Set operation
  if (best_var_idx != SIZE_MAX)
  {
    ctx.m_op_size = 1;
    ctx.m_op_var_idxs[0] = best_var_idx;
    ctx.m_op_var_deltas[0] = best_delta;

    if (stats)
    {
      stats->successful_ops++;
      stats->gradient_steps++;
    }
  }
}

int main()
{
  Local_MIP solver;
  NeighborStats stats;

  // Clear default neighbors
  solver.clear_neighbor_list();

  // Add custom neighbors with user data
  solver.add_custom_neighbor("smart_flip", smart_flip_neighbor, &stats);
  solver.add_custom_neighbor("greedy_gradient", greedy_gradient_neighbor, &stats);

  // Can also mix with predefined neighbors
  solver.add_neighbor("flip", 0, 12);

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(30.0);
  solver.run();

  // Print statistics
  printf("Total calls: %zu\n", stats.total_calls);
  printf("Binary flips: %zu\n", stats.binary_flips);
  printf("Gradient steps: %zu\n", stats.gradient_steps);

  return 0;
}
```

## Context API

The neighbor receives `Neighbor::Neighbor_Ctx` with:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| **Outputs** | | | |
| `m_op_size` | `size_t&` | W | Number of variables in move (0-N) |
| `m_op_var_idxs` | `size_t[]` | W | Variable indices to change |
| `m_op_var_deltas` | `double[]` | W | Delta values for each variable |
| **Inputs** | | | |
| `m_rng` | `mt19937&` | R/W | Random number generator |
| `m_shared.m_var_current_value` | `vector<double>` | R | Current variable values |
| `m_shared.m_var_obj_cost` | `vector<double>` | R | Objective coefficients |
| `m_shared.m_binary_idx_list` | `vector<size_t>` | R | Binary variable indices |
| `m_shared.m_non_fixed_var_idx_list` | `vector<size_t>` | R | Non-fixed variable indices |
| `m_shared.m_is_found_feasible` | `bool` | R | Feasibility status |
| `m_shared.m_model_manager` | `Model_Manager&` | R | Model metadata |

## Building & Running

```bash
cd example/neighbor-userdata
g++ -O3 -std=c++20 neighbor_userdata.cpp -I../../src -L../../build -lLocalMIP -lpthread -o neighbor_userdata_demo
./neighbor_userdata_demo
```

## Expected Output

```
========== Neighbor User Data Example ==========

Instance: test-set/2club200v15p5scn.mps

Add custom Neighbors with statistics:
  - smart_flip: smart flip binary variables
  - greedy_gradient: greedy gradient descent

Start solving...

c model name: 2club200v15p5scn
...
c [     30.00] local search is terminated by timeout.
o best objective: -45

========== Neighbor Statistics ==========
Total calls:     632368
Successful operations:   632368
Failed operations:   0
Binary variable flips:   317038
Gradient descent steps:   315330

Success rate: 100.00%

Solution status: Feasible
Objective: -45.000000
```

## Key Points

1. **Function Signature**: `void neighbor(Neighbor::Neighbor_Ctx& ctx, void* user_data)`
2. **Operation Output**: Set `ctx.m_op_size` and fill `m_op_var_idxs[]`, `m_op_var_deltas[]`
3. **No Operation**: Set `ctx.m_op_size = 0` if no move found
4. **Multi-Variable Moves**: Can modify multiple variables in one operation
5. **User Data**: Pass any structure via `void*` pointer (shared across calls)
6. **Mix Operations**: Can combine custom and predefined neighbors

## Configuration API

```cpp
// Clear default neighbor list
solver.clear_neighbor_list();

// Add custom neighbor
solver.add_custom_neighbor("name", function, user_data_ptr);

// Add predefined neighbor (see neighbor-config example)
solver.add_neighbor("flip", bms_con, bms_op);

// Reset to default configuration
solver.reset_default_neighbor_list();
```

## Common Use Cases

- **Problem-specific operators**: Exploit domain knowledge
- **Multi-variable moves**: Swap, exchange, shift operations
- **Adaptive strategies**: Change behavior based on search state
- **Statistics tracking**: Count operation types, success rates
- **Hybrid methods**: Combine heuristics and random search

## Related Examples

- `neighbor-config/` - Configure predefined neighborhood operations
- `scoring-neighbor/` - Random tie-breaking in scoring
- `start-callback/` - Custom initialization
- `restart-callback/` - Custom restart strategies
