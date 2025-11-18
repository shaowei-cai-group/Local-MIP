# Restart Callback Example

Demonstrates how to customize restart strategies using a callback function to escape local optima.

## What It Does

This example shows how to:
- Define a custom restart callback to control restart behavior
- Reset constraint weights during restart
- Perturb the current solution to explore different regions
- Track restart invocations with user data

## Why Use Restart Callbacks?

Restart strategies help:
- Escape local optima by diversifying the search
- Reset accumulated constraint weights
- Implement problem-specific perturbation strategies
- Balance intensification and diversification

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;
  int restart_count = 0;

  // Define custom restart callback
  Local_Search::Restart_Cbk restart_cbk =
      [](Restart::Restart_Ctx& ctx, void* p_user_data)
  {
    auto& values = ctx.m_var_current_value;       // Current variable values (R/W)
    auto& weights = ctx.m_con_weight;             // Constraint weights (R/W)
    auto& rng = ctx.m_rng;                        // Random number generator
    const auto& binary_vars = ctx.m_shared.m_binary_idx_list;
    const auto& best_values = ctx.m_shared.m_var_best_value;

    // Track restarts
    if (p_user_data)
    {
      int* counter = static_cast<int*>(p_user_data);
      (*counter)++;
      printf("=== Restart #%d ===\n", *counter);
    }

    // Strategy:
    // 1. Reset all constraint weights to 1
    for (size_t i = 0; i < weights.size(); ++i)
    {
      weights[i] = 1;
    }

    // 2. Start from best solution (if feasible found)
    if (ctx.m_shared.m_is_found_feasible)
      for (size_t i = 0; i < values.size(); ++i)
      {
        values[i] = best_values[i];
      }

    // 3. Randomly flip 20% of binary variables for perturbation
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    for (size_t var_idx : binary_vars)
    {
      if (prob_dist(rng) < 0.2)
      {
        values[var_idx] = (values[var_idx] > 0.5) ? 0.0 : 1.0;
      }
    }
  };

  // Register callback
  solver.set_restart_cbk(restart_cbk, &restart_count);
  solver.set_restart_step(5000);  // Restart every 5000 non-improving steps

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(60.0);
  solver.run();

  return 0;
}
```

## Context API

The callback receives `Restart::Restart_Ctx` with access to:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| `m_var_current_value` | `vector<double>&` | R/W | Current variable values |
| `m_con_weight` | `vector<size_t>&` | R/W | Constraint weights |
| `m_rng` | `mt19937&` | R/W | Random number generator |
| `m_shared.m_var_best_value` | `vector<double>` | R | Best solution found |
| `m_shared.m_is_found_feasible` | `bool` | R | Whether feasible solution exists |
| `m_shared.m_binary_idx_list` | `vector<size_t>` | R | Binary variable indices |

## Building & Running

```bash
cd example/restart-callback
g++ -O3 -std=c++20 restart_callback.cpp -I../../src -L../../build -lLocalMIP -lpthread -o restart_callback_demo
cd ..  # run from example/ so test-set/ is in place
./restart-callback/restart_callback_demo
```

## Expected Output

```
c custom restart callback is registered.
c restart step is set to : 5000
...
c [      0.10] obj*: -36
c [      2.00] obj*: -57
c [      2.70] obj*: -69
=== Restart #1 ===
Restart: Reset weights, flipped 31/200 binary variables (15.5%)
=== Restart #2 ===
Restart: Reset weights, flipped 35/200 binary variables (17.5%)
...
=== Restart #15 ===
Restart: Reset weights, flipped 48/200 binary variables (24.0%)
o best objective: -69
Solution is feasible!
Objective value: -69.0000000000
```

## Key Points

1. **Callback Signature**: `void callback(Restart::Restart_Ctx& ctx, void* user_data)`
2. **Invocation Timing**: Triggered after N non-improving steps (set by `set_restart_step()`)
3. **Weight Reset**: Common strategy to reset constraint weights to initial values
4. **Perturbation**: Modify `ctx.m_var_current_value` to diversify search
5. **Best Solution Access**: Use `ctx.m_shared.m_var_best_value` to restart from best known solution

## Common Restart Strategies

- **Random Restart**: Completely randomize variable values
- **Best + Perturbation**: Start from best solution with small random changes (shown here)
- **Weight Reset Only**: Reset weights but keep current solution
- **Hybrid**: Combine multiple strategies based on search progress

## Related Examples

- `start-callback/` - Custom initialization
- `weight-callback/` - Custom weight update strategies
- `neighbor-config/` - Custom neighborhood operations
