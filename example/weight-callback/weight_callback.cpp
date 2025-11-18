/*=====================================================================================

    Filename:     weight_callback.cpp

    Description:  Weight Callback Example - Demonstrates custom constraint weight update control via callback
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
  struct WeightStats
  {
    int total_calls = 0;
    int triggered_updates = 0;
  };
  WeightStats stats;

  // Custom weight update callback: probabilistic monotone increase (0.5 probability)
  // Note: callback now accepts void* p_user_data parameter
  Local_Search::Weight_Cbk weight_cbk =
      [](Weight::Weight_Ctx& ctx, void* p_user_data)
  {
    auto& weights = ctx.m_con_weight;
    auto& rng = ctx.m_rng;
    const auto& unsat_idxs = ctx.m_shared.m_con_unsat_idxs;

    // Use user_data for statistics
    WeightStats* pstats = nullptr;
    if (p_user_data)
    {
      pstats = static_cast<WeightStats*>(p_user_data);
      pstats->total_calls++;
    }

    // Use uniform distribution to generate random number between 0.0 and 1.0
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

    // 0.5 probability to increase weights of unsatisfied constraints
    if (prob_dist(rng) < 0.5)
    {
      if (pstats)
        pstats->triggered_updates++;

      size_t updated_count = 0;

      // Increase weights for all unsatisfied constraints
      for (size_t con_idx : unsat_idxs)
      {
        weights[con_idx]++;
        updated_count++;
      }

      // If feasible solution found and all constraints satisfied, increase objective weight
      if (ctx.m_shared.m_is_found_feasible && unsat_idxs.empty())
      {
        weights[0]++;
        updated_count++;
      }

      if (updated_count > 0 && pstats && pstats->total_calls % 1000 == 0)
      {
        printf("Weight: Call #%d, triggered %d/%d times (%.1f%%)\n",
               pstats->total_calls,
               pstats->triggered_updates,
               pstats->total_calls,
               100.0 * pstats->triggered_updates / pstats->total_calls);
      }
    }
  };

  // Pass user_data pointer
  solver.set_weight_cbk(weight_cbk, &stats);
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_weight.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_weight.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  return 0;
}
