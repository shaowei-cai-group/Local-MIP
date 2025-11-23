# Neighbor Random Example

Demonstrates how to customize neighbor scoring with random tie-breaking in the infeasible phase.

## What It Does

This example shows how to:
- Define a custom neighbor scoring callback for infeasible phase search
- Implement progress-based scoring with random tie-breaking
- Calculate constraint violation improvements
- Track extensive scoring statistics

## Why Use Neighbor Scoring Callbacks?

Custom neighbor scoring can:
- Implement problem-specific move selection heuristics
- Control tie-breaking behavior (age, random, etc.)
- Balance intensification and diversification
- Improve convergence in the infeasible search phase

## Neighbor vs Lift Scoring

| Aspect | Neighbor Scoring | Lift Scoring |
|--------|------------------|--------------|
| **Phase** | Infeasible (finding feasibility) | Feasible (optimizing objective) |
| **Goal** | Reduce constraint violations | Improve objective value |
| **Frequency** | Very high (millions of calls) | Moderate (thousands of calls) |
| **Complexity** | Higher (all constraints) | Lower (objective focus) |

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;

  // User statistics with RNG
  struct NeighborStats
  {
    int total_neighbor_calls = 0;
    int random_tie_breaks = 0;
    int score_improvements = 0;
    std::mt19937 rng;  // Random number generator

    NeighborStats() : rng(std::random_device{}()) {}
  };
  NeighborStats stats;

  // Custom neighbor scoring callback
  Local_Search::Neighbor_Scoring_Cbk neighbor_cbk =
      [](Scoring::Neighbor_Ctx& ctx, size_t p_var_idx, double p_delta, void* p_user_data)
  {
    NeighborStats* pstats = static_cast<NeighborStats*>(p_user_data);
    if (pstats) pstats->total_neighbor_calls++;

    // Calculate progress score (constraint violation reduction)
    long neighbor_score = 0;
    long bonus_score = 0;

    // Iterate through all constraints involving this variable
    auto& model_var = ctx.m_shared.m_model_manager.var(p_var_idx);
    for (size_t term_idx = 0; term_idx < model_var.term_num(); ++term_idx)
    {
      size_t con_idx = model_var.con_idx(term_idx);
      // ... calculate violation changes ...
      // Increase score if violation reduced
      // Decrease score if violation increased
    }

    // Three-level tie-breaking:
    bool should_update = false;

    // Level 1: Better progress score
    if (ctx.m_best_neighbor_score < neighbor_score)
    {
      should_update = true;
      if (pstats) pstats->score_improvements++;
    }
    // Level 2: Same score, better bonus (breakthrough potential)
    else if (ctx.m_best_neighbor_score == neighbor_score &&
             ctx.m_best_neighbor_subscore < bonus_score)
    {
      should_update = true;
    }
    // Level 3: All equal - use random tie-breaking (50% probability)
    else if (ctx.m_best_neighbor_score == neighbor_score &&
             ctx.m_best_neighbor_subscore == bonus_score)
    {
      if (ctx.m_best_var_idx == SIZE_MAX || (pstats && (pstats->rng() & 1U)))
      {
        should_update = true;
        if (pstats && ctx.m_best_var_idx != SIZE_MAX)
          pstats->random_tie_breaks++;
      }
    }

    // Update best variable
    if (should_update)
    {
      ctx.m_best_var_idx = p_var_idx;
      ctx.m_best_delta = p_delta;
      ctx.m_best_neighbor_score = neighbor_score;
      ctx.m_best_neighbor_subscore = bonus_score;
    }
  };

  // Register callback
  solver.set_neighbor_scoring_cbk(neighbor_cbk, &stats);

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(60.0);
  solver.run();

  return 0;
}
```

## Context API

The callback receives `Scoring::Neighbor_Ctx` with access to:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| `m_best_var_idx` | `size_t&` | R/W | Best variable index |
| `m_best_delta` | `double&` | R/W | Best delta value |
| `m_best_neighbor_score` | `long&` | R/W | Best progress score |
| `m_best_neighbor_subscore` | `long&` | R/W | Best bonus score (breakthrough) |
| `m_best_age` | `size_t&` | R/W | Best variable age |
| `m_binary_op_stamp` | `vector<uint32_t>&` | R/W | Binary duplicate detection |
| `m_binary_op_stamp_token` | `uint32_t` | R | Current token |
| `m_shared.m_con_weight` | `vector<size_t>` | R | Constraint weights |
| `m_shared.m_con_activity` | `vector<double>` | R | Constraint activities |
| `m_shared.m_con_constant` | `vector<double>` | R | Constraint RHS |
| `m_shared.m_is_found_feasible` | `bool` | R | Feasibility status |
| `m_shared.m_best_obj` | `double` | R | Best objective value |

## Building & Running

```bash
cd example/scoring-neighbor
g++ -O3 -std=c++20 neighbor_random.cpp -I../../src -L../../build -lLocalMIP -lpthread -o neighbor_random_demo
./neighbor_random_demo
```

## Expected Output

```
c custom neighbor scoring callback is registered.
c model name: 2club200v15p5scn
...
c [      0.10] obj*: -28
Neighbor: 50000 calls, 1623 score improvements, 596 random tie-breaks (1.2%)
Neighbor: 100000 calls, 3062 score improvements, 947 random tie-breaks (0.9%)
c [      1.00] obj*: -57
c [      1.40] obj*: -69
...
Neighbor: 9150000 calls, 120563 score improvements, 3019 random tie-breaks (0.0%)
o best objective: -69

=== Final Statistics ===
Total neighbor evaluations: 9178155
Score improvements: 120829 (1.3%)
Random tie-breaks: 3021 (0.0%)
```

## Key Points

1. **Callback Signature**: `void callback(Scoring::Neighbor_Ctx& ctx, size_t var_idx, double delta, void* user_data)`
2. **Invocation Phase**: Only called during infeasible phase (finding feasibility)
3. **Call Frequency**: Extremely high (millions of calls per run)
4. **Progress Score**: Weighted sum of constraint violation changes
5. **Bonus Score**: Extra points for achieving new best objective
6. **Binary Deduplication**: Use stamp mechanism to avoid duplicate evaluations

## Three-Level Tie-Breaking

1. **Primary**: Progress score (constraint violation reduction)
   - Higher score = more violations fixed
2. **Secondary**: Bonus score (breakthrough potential)
   - Extra points for beating best objective
3. **Tertiary**: Random (50% probability)
   - Provides diversification without bias

## Performance Notes

- This callback is performance-critical (millions of calls)
- Avoid expensive computations inside the callback
- Use efficient data structures
- Binary stamp mechanism prevents duplicate work

## Related Examples

- `scoring-lift/` - Degree-based lift scoring (feasible phase)
- `neighbor-config/` - Configure predefined neighborhood operations
- `neighbor-userdata/` - Complex user data structures
