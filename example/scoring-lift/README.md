# Lift Degree Example

Demonstrates how to customize lift scoring with degree-based tie-breaking in the feasible phase.

## What It Does

This example shows how to:
- Define a custom lift scoring callback for feasible solution optimization
- Use variable degree (constraint count) as a tie-breaking criterion
- Implement multi-level tie-breaking strategies
- Track scoring statistics with user data

## Why Use Lift Scoring Callbacks?

Custom lift scoring can:
- Implement problem-specific variable selection heuristics
- Control tie-breaking behavior (age, degree, random, etc.)
- Prioritize variables based on structural properties
- Improve convergence in the feasible optimization phase

## What is "Degree"?

**Degree** = Number of constraints a variable appears in
- Low degree variables affect fewer constraints (localized impact)
- High degree variables affect many constraints (global impact)
- Degree-based selection can balance exploration and stability

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;

  // User statistics
  struct LiftStats
  {
    int total_lift_calls = 0;
    int degree_tie_breaks = 0;
    int score_improvements = 0;
  };
  LiftStats stats;

  // Custom lift scoring callback
  Local_Search::Lift_Scoring_Cbk lift_cbk =
      [](Scoring::Lift_Ctx& ctx, size_t p_var_idx, double p_delta, void* p_user_data)
  {
    // Calculate lift score
    double lift_score = -ctx.m_shared.m_var_obj_cost[p_var_idx] * p_delta;

    // Get variable age
    size_t age = std::max(ctx.m_shared.m_var_last_dec_step[p_var_idx],
                          ctx.m_shared.m_var_last_inc_step[p_var_idx]);

    // Get variable degree (constraint count)
    size_t degree = ctx.m_shared.m_model_manager.var(p_var_idx).term_num();

    // Track statistics
    LiftStats* pstats = static_cast<LiftStats*>(p_user_data);
    if (pstats) pstats->total_lift_calls++;

    // Three-level tie-breaking strategy:
    bool should_update = false;

    // Level 1: Better lift score (significant improvement)
    if (ctx.m_best_lift_score + k_opt_tolerance < lift_score)
    {
      should_update = true;
      if (pstats) pstats->score_improvements++;
    }
    // Level 2: Similar lift score - use degree to break tie
    else if (ctx.m_best_lift_score <= lift_score)
    {
      if (ctx.m_best_var_idx == SIZE_MAX)
      {
        should_update = true;  // First candidate
      }
      else
      {
        size_t best_degree = ctx.m_shared.m_model_manager.var(ctx.m_best_var_idx).term_num();

        // Prefer smaller degree (fewer constraints, localized impact)
        if (degree < best_degree)
        {
          should_update = true;
          if (pstats) pstats->degree_tie_breaks++;
        }
        // Level 3: Same degree - use age to break tie
        else if (degree == best_degree && age < ctx.m_best_age)
        {
          should_update = true;
        }
      }
    }

    // Update best variable
    if (should_update)
    {
      ctx.m_best_var_idx = p_var_idx;
      ctx.m_best_delta = p_delta;
      ctx.m_best_lift_score = lift_score;
      ctx.m_best_age = age;
    }
  };

  // Register callback
  solver.set_lift_scoring_cbk(lift_cbk, &stats);

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(60.0);
  solver.run();

  return 0;
}
```

## Context API

The callback receives `Scoring::Lift_Ctx` with access to:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| `m_best_var_idx` | `size_t&` | R/W | Best variable index to update |
| `m_best_delta` | `double&` | R/W | Best delta value |
| `m_best_lift_score` | `double&` | R/W | Best lift score |
| `m_best_age` | `size_t&` | R/W | Best variable age |
| `m_shared.m_var_obj_cost` | `vector<double>` | R | Variable objective coefficients |
| `m_shared.m_var_last_dec_step` | `vector<size_t>` | R | Last decrease step |
| `m_shared.m_var_last_inc_step` | `vector<size_t>` | R | Last increase step |
| `m_shared.m_model_manager` | `Model_Manager&` | R | Model metadata (for degree) |

## Building & Running

```bash
cd example/scoring-lift
g++ -O3 -std=c++20 lift_degree.cpp -I../../src -L../../build -lLocalMIP -lpthread -o lift_degree_demo
cd ..  # run from example/ so test-set/ is in place
./scoring-lift/lift_degree_demo
```

## Expected Output

```
c custom lift scoring callback is registered.
c model name: 2club200v15p5scn
...
c [      0.10] obj*: -14
c [      1.30] obj*: -49
c [      2.30] obj*: -59
c [      6.11] obj*: -68
c [      6.91] obj*: -69
Lift: 10000 calls, 17 score improvements, 156 degree tie-breaks (1.6%)
Lift: 20000 calls, 17 score improvements, 306 degree tie-breaks (1.5%)
o best objective: -69

=== Final Statistics ===
Total lift evaluations: 24000
Score improvements: 17 (0.1%)
Degree-based tie-breaks: 366 (1.5%)
```

## Key Points

1. **Callback Signature**: `void callback(Scoring::Lift_Ctx& ctx, size_t var_idx, double delta, void* user_data)`
2. **Invocation Phase**: Only called during feasible optimization phase
3. **Lift Score**: Typically `-obj_coeff * delta` (objective improvement)
4. **Degree Access**: Use `model_manager.var(idx).term_num()` to get constraint count
5. **Callback Frequency**: Called for each candidate variable (thousands of times)

## Three-Level Tie-Breaking Strategy

1. **Primary**: Lift score (objective improvement)
   - Accept if significantly better than current best
2. **Secondary**: Degree (constraint count)
   - When lift scores similar, prefer lower degree (localized changes)
3. **Tertiary**: Age (tabu-based)
   - When degrees equal, prefer less recently changed variables

## Common Tie-Breaking Strategies

- **Age-based**: Prefer less recently moved variables (tabu-like)
- **Degree-based**: Prefer low/high degree variables (shown here)
- **Random**: Break ties randomly for diversification
- **Hybrid**: Combine multiple criteria with priorities

## Related Examples

- `scoring-neighbor/` - Random-based neighbor selection
- `neighbor-config/` - Custom neighborhood configuration
- `start-callback/` - Custom initialization
- `restart-callback/` - Custom restart strategies
