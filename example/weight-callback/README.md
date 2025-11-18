# Weight Callback Example

Demonstrates how to customize constraint weight update strategies using a callback function.

## What It Does

This example shows how to:
- Define a custom weight callback to control constraint weight updates
- Implement probabilistic weight increase strategies
- Track weight update statistics with user data
- Handle both infeasible and feasible phases

## Why Use Weight Callbacks?

Custom weight strategies can:
- Implement problem-specific penalty schemes
- Balance constraint satisfaction priorities
- Control the trade-off between feasibility and optimality
- Experiment with different weight update policies (smooth vs monotone)

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;

  // User-defined statistics structure
  struct WeightStats
  {
    int total_calls = 0;
    int triggered_updates = 0;
  };
  WeightStats stats;

  // Define custom weight callback
  Local_Search::Weight_Cbk weight_cbk =
      [](Weight::Weight_Ctx& ctx, void* p_user_data)
  {
    auto& weights = ctx.m_con_weight;             // Constraint weights (R/W)
    auto& rng = ctx.m_rng;                        // Random number generator
    const auto& unsat_idxs = ctx.m_shared.m_con_unsat_idxs;

    // Track statistics
    WeightStats* pstats = static_cast<WeightStats*>(p_user_data);
    if (pstats) pstats->total_calls++;

    // Probabilistic weight increase (50% probability)
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    if (prob_dist(rng) < 0.5)
    {
      if (pstats) pstats->triggered_updates++;

      // Increase weights for all unsatisfied constraints
      for (size_t con_idx : unsat_idxs)
      {
        weights[con_idx]++;
      }

      // If feasible and all constraints satisfied, increase objective weight
      if (ctx.m_shared.m_is_found_feasible && unsat_idxs.empty())
      {
        weights[0]++;  // Objective function is constraint 0
      }
    }
  };

  // Register callback
  solver.set_weight_cbk(weight_cbk, &stats);

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(60.0);
  solver.run();

  return 0;
}
```

## Context API

The callback receives `Weight::Weight_Ctx` with access to:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| `m_con_weight` | `vector<size_t>&` | R/W | Constraint weights (index 0 = objective) |
| `m_rng` | `mt19937&` | R/W | Random number generator |
| `m_shared.m_con_unsat_idxs` | `vector<size_t>` | R | Unsatisfied constraint indices |
| `m_shared.m_is_found_feasible` | `bool` | R | Whether feasible solution found |
| `m_shared.m_con_activity` | `vector<double>` | R | Constraint activity values |
| `m_shared.m_con_constant` | `vector<double>` | R | Constraint RHS values |

## Building & Running

```bash
cd example/weight-callback
g++ -O3 -std=c++20 weight_callback.cpp -I../../src -L../../build -lLocalMIP -lpthread -o weight_callback_demo
cd ..  # run from example/ so test-set/ is in place
./weight-callback/weight_callback_demo
```

## Expected Output

```
c custom weight callback is registered.
c model name: 2club200v15p5scn
...
c [      0.10] obj*: -36
c [      1.40] obj*: -62
Weight: Call #1000, triggered 509/1000 times (50.9%)
c [      2.40] obj*: -65
...
Weight: Call #8000, triggered 4010/8000 times (50.1%)
Weight: Call #13000, triggered 6556/13000 times (50.4%)
...
Weight: Call #32000, triggered 16053/32000 times (50.2%)
o best objective: -69
Solution is feasible!
Objective value: -69.0000000000
```

## Key Points

1. **Callback Signature**: `void callback(Weight::Weight_Ctx& ctx, void* user_data)`
2. **Invocation Frequency**: Called frequently during search (thousands of times)
3. **Weight Index 0**: Always represents the objective function
4. **Infeasible Phase**: Increase weights of unsatisfied constraints
5. **Feasible Phase**: Can increase objective weight when all constraints satisfied

## Weight Update Strategies

### Built-in Methods
- **Smooth**: Weights can increase or decrease (default)
- **Monotone**: Weights only increase

### Custom Strategies (via callback)
- **Probabilistic**: Random probability-based updates (shown here)
- **Adaptive**: Adjust based on constraint violation degree
- **Priority-based**: Different weights for different constraint types
- **Age-based**: Increase weights for long-unsatisfied constraints

## Important Notes

- Weight updates directly affect the scoring function used in move selection
- Higher weights make satisfying those constraints more important
- Constraint 0 is always the objective function
- Be careful with weight overflow for long runs

## Related Examples

- `start-callback/` - Custom initialization
- `restart-callback/` - Custom restart strategies
- `neighbor-config/` - Custom neighborhood operations
