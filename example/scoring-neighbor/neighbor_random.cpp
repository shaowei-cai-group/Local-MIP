/*=====================================================================================

    Filename:     neighbor_random.cpp

    Description:  Random Neighbor Example - Demonstrates neighbor_random operation usage
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include "model_data/Model_Con.h"
#include "model_data/Model_Manager.h"
#include "model_data/Model_Var.h"
#include "utils/global_defs.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

int main()
{
  Local_MIP solver;

  // User-defined data: statistics structure(includes random number generator)
  struct NeighborStats
  {
    int total_neighbor_calls = 0;
    int random_tie_breaks = 0;
    int score_improvements = 0;
    std::mt19937 rng; // Random number generator

    NeighborStats() : rng(std::random_device{}()) {}
  };
  NeighborStats stats;

  // Custom Neighbor scoring callback: progress_random (use random to break ties)
  // Note: callback now accepts void* p_user_data parameter
  Local_Search::Neighbor_Scoring_Cbk neighbor_cbk =
      [](Scoring::Neighbor_Ctx& ctx, size_t p_var_idx, double p_delta, void* p_user_data)
  {
    // Use user_data for statistics
    NeighborStats* pstats = nullptr;
    if (p_user_data)
    {
      pstats = static_cast<NeighborStats*>(p_user_data);
      pstats->total_neighbor_calls++;
    }

    // Calculate neighbor_score (same logic as progress_bonus)
    auto& model_var = ctx.m_shared.m_model_manager.var(p_var_idx);

    // For binary variables, use stamp to avoid duplicate evaluation
    if (model_var.is_binary())
    {
      if (ctx.m_binary_op_stamp[p_var_idx] == ctx.m_binary_op_stamp_token)
        return;
      ctx.m_binary_op_stamp[p_var_idx] = ctx.m_binary_op_stamp_token;
    }

    long neighbor_score = 0;
    long bonus_score = 0;
    size_t term_num = model_var.term_num();
    if (term_num == 0)
      return;

    // Calculatecontribution of each constraint
    for (size_t term_idx = 0; term_idx < term_num; ++term_idx)
    {
      size_t con_idx = model_var.con_idx(term_idx);
      size_t pos_in_con = model_var.pos_in_con(term_idx);
      auto& model_con = ctx.m_shared.m_model_manager.con(con_idx);

      // Handle objective function constraint (index 0)
      if (con_idx == 0 && ctx.m_shared.m_is_found_feasible)
      {
        long double ld_new_obj =
            static_cast<long double>(ctx.m_shared.m_con_activity[con_idx]) +
            static_cast<long double>(model_con.coeff(pos_in_con)) *
                static_cast<long double>(p_delta);
        double new_obj = static_cast<double>(ld_new_obj);

        if (new_obj < ctx.m_shared.m_con_activity[con_idx])
          neighbor_score += ctx.m_shared.m_con_weight[con_idx];
        else
          neighbor_score -= ctx.m_shared.m_con_weight[con_idx];

        if (new_obj < ctx.m_shared.m_best_obj)
          bonus_score += ctx.m_shared.m_con_weight[con_idx];
      }
      // Handle normal constraints
      else
      {
        long double ld_new_activity =
            static_cast<long double>(ctx.m_shared.m_con_activity[con_idx]) +
            static_cast<long double>(model_con.coeff(pos_in_con)) *
                static_cast<long double>(p_delta);
        double new_activity = static_cast<double>(ld_new_activity);

        long double ld_pre_gap =
            static_cast<long double>(ctx.m_shared.m_con_activity[con_idx]) -
            static_cast<long double>(ctx.m_shared.m_con_constant[con_idx]);
        double pre_gap = static_cast<double>(ld_pre_gap);

        long double ld_new_gap =
            ld_new_activity -
            static_cast<long double>(ctx.m_shared.m_con_constant[con_idx]);
        double new_gap = static_cast<double>(ld_new_gap);

        bool pre_sat;
        // Equality constraint
        if (ctx.m_shared.m_con_is_equality[con_idx])
        {
          pre_sat = std::fabs(pre_gap) <= k_feas_tolerance;
          bool now_sat = std::fabs(new_gap) <= k_feas_tolerance;

          if (!pre_sat && now_sat)
            neighbor_score += ctx.m_shared.m_con_weight[con_idx] * 2;
          else if (pre_sat && !now_sat)
            neighbor_score -= ctx.m_shared.m_con_weight[con_idx] * 2;
          else if (!pre_sat && !now_sat)
          {
            if (std::fabs(new_gap) < std::fabs(pre_gap))
              neighbor_score += ctx.m_shared.m_con_weight[con_idx];
            else
              neighbor_score -= ctx.m_shared.m_con_weight[con_idx];
          }
        }
        // Inequality constraint
        else
        {
          pre_sat = pre_gap <= k_feas_tolerance;
          bool now_sat = new_gap <= k_feas_tolerance;

          if (!pre_sat && now_sat)
            neighbor_score += ctx.m_shared.m_con_weight[con_idx];
          else if (pre_sat && !now_sat)
            neighbor_score -= ctx.m_shared.m_con_weight[con_idx];
          else if (!pre_sat && !now_sat)
          {
            if (new_gap < pre_gap)
              neighbor_score += ctx.m_shared.m_con_weight[con_idx] >> 1;
            else
              neighbor_score -= ctx.m_shared.m_con_weight[con_idx] >> 1;
          }
        }
      }
    }

    size_t age = std::max(ctx.m_shared.m_var_last_dec_step[p_var_idx],
                          ctx.m_shared.m_var_last_inc_step[p_var_idx]);

    // Check if best variable needs updating
    bool should_update = false;

    // 1. If neighbor score better
    if (ctx.m_best_neighbor_score < neighbor_score)
    {
      should_update = true;
      if (pstats)
        pstats->score_improvements++;
    }
    // 2. If neighbor score same, use bonus_score as secondary criterion
    else if (ctx.m_best_neighbor_score == neighbor_score &&
             ctx.m_best_neighbor_subscore < bonus_score)
    {
      should_update = true;
      if (pstats)
        pstats->score_improvements++;
    }
    // 3. If neighbor score and bonus_score both same, use random to break tie
    else if (ctx.m_best_neighbor_score == neighbor_score &&
             ctx.m_best_neighbor_subscore == bonus_score)
    {
      // Ifis first candidate variable, accept directly
      if (ctx.m_best_var_idx == SIZE_MAX)
      {
        should_update = true;
      }
      else
      {
        // UseUse random to break tie (50% probability to accept new variable)
        if (pstats && (pstats->rng() & 1U) != 0)
        {
          should_update = true;
          pstats->random_tie_breaks++;
        }
      }
    }

    // Update best variable
    if (should_update)
    {
      ctx.m_best_var_idx = p_var_idx;
      ctx.m_best_delta = p_delta;
      ctx.m_best_neighbor_score = neighbor_score;
      ctx.m_best_neighbor_subscore = bonus_score;
      ctx.m_best_age = age;
    }

    // Periodic output statistics
    if (pstats && pstats->total_neighbor_calls % 50000 == 0)
    {
      printf("Neighbor: %d calls, %d score improvements, %d random tie-breaks (%.1f%%)\n",
             pstats->total_neighbor_calls,
             pstats->score_improvements,
             pstats->random_tie_breaks,
             100.0 * pstats->random_tie_breaks / pstats->total_neighbor_calls);
    }
  };

  // Pass user_data pointer
  solver.set_neighbor_scoring_cbk(neighbor_cbk, &stats);
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_neighbor_random.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_neighbor_random.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  // Output final statistics
  printf("\n=== Final Statistics ===\n");
  printf("Total neighbor evaluations: %d\n", stats.total_neighbor_calls);
  printf("Score improvements: %d (%.1f%%)\n",
         stats.score_improvements,
         100.0 * stats.score_improvements / stats.total_neighbor_calls);
  printf("Random tie-breaks: %d (%.1f%%)\n",
         stats.random_tie_breaks,
         100.0 * stats.random_tie_breaks / stats.total_neighbor_calls);

  return 0;
}
