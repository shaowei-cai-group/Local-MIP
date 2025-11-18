/*=====================================================================================

    Filename:     lift_degree.cpp

    Description:  Lift Degree Example - Demonstrates lift_degree scoring method usage
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include <cstdio>
#include <random>

int main()
{
  Local_MIP solver;

  // User-defined data: statistics structure
  struct LiftStats
  {
    int total_lift_calls = 0;
    int degree_tie_breaks = 0;
    int score_improvements = 0;
  };
  LiftStats stats;

  // Custom Lift scoring callback: use degree (variable term count) to break ties
  // Note: callback now accepts void* p_user_data parameter
  Local_Search::Lift_Scoring_Cbk lift_cbk =
      [](Scoring::Lift_Ctx& ctx, size_t p_var_idx, double p_delta, void* p_user_data)
  {
    // Calculate lift score (same as standard lift_age)
    double lift_score = -ctx.m_shared.m_var_obj_cost[p_var_idx] * p_delta;

    // Get variable age
    size_t age = std::max(ctx.m_shared.m_var_last_dec_step[p_var_idx],
                          ctx.m_shared.m_var_last_inc_step[p_var_idx]);

    // Get variable degree (number of constraints it appears in)
    size_t degree = ctx.m_shared.m_model_manager.var(p_var_idx).term_num();

    // Use user_data for statistics
    LiftStats* pstats = nullptr;
    if (p_user_data)
    {
      pstats = static_cast<LiftStats*>(p_user_data);
      pstats->total_lift_calls++;
    }

    // Check if best variable needs updating
    bool should_update = false;

    // 1. If lift score significantly better (exceeds tolerance)
    if (ctx.m_best_lift_score + k_opt_tolerance < lift_score)
    {
      should_update = true;
      if (pstats)
        pstats->score_improvements++;
    }
    // 2. If lift score similar (within tolerance), use degree to break tie
    else if (ctx.m_best_lift_score <= lift_score)
    {
      // Ifis first candidate variable, accept directly
      if (ctx.m_best_var_idx == SIZE_MAX)
      {
        should_update = true;
      }
      else
      {
        // Get current best variable degree
        size_t best_degree = ctx.m_shared.m_model_manager.var(ctx.m_best_var_idx).term_num();

        // UseThree-level tie-breaking strategy:
        // Level 1: Prefer variables with smaller degree (fewer constraints, smaller impact)
        // Level 2: If degree same, prefer variables with smaller age
        // Level 3: If age also same, keep original choice (no update)
        if (degree < best_degree)
        {
          should_update = true;
          if (pstats)
            pstats->degree_tie_breaks++;
        }
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

    // Periodic output statistics
    if (pstats && pstats->total_lift_calls % 10000 == 0)
    {
      printf("Lift: %d calls, %d score improvements, %d degree tie-breaks (%.1f%%)\n",
             pstats->total_lift_calls,
             pstats->score_improvements,
             pstats->degree_tie_breaks,
             100.0 * pstats->degree_tie_breaks / pstats->total_lift_calls);
    }
  };

  // Pass user_data pointer
  solver.set_lift_scoring_cbk(lift_cbk, &stats);
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_lift_degree.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_lift_degree.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  // Output final statistics
  printf("\n=== Final Statistics ===\n");
  printf("Total lift evaluations: %d\n", stats.total_lift_calls);
  printf("Score improvements: %d (%.1f%%)\n",
         stats.score_improvements,
         100.0 * stats.score_improvements / stats.total_lift_calls);
  printf("Degree-based tie-breaks: %d (%.1f%%)\n",
         stats.degree_tie_breaks,
         100.0 * stats.degree_tie_breaks / stats.total_lift_calls);

  return 0;
}
