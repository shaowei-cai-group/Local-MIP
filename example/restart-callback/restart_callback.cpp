/*=====================================================================================

    Filename:     restart_callback.cpp

    Description:  Restart Callback Example - Demonstrates custom restart strategy control via callback
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

  // User-defined data: restart counter
  int restart_count = 0;

  // Custom restart callback: reset weights and perturb variable values
  // Note: callback now accepts void* p_user_data parameter
  Local_Search::Restart_Cbk restart_cbk =
      [](Restart::Restart_Ctx& ctx, void* p_user_data)
  {
    auto& values = ctx.m_var_current_value;
    auto& weights = ctx.m_con_weight;
    auto& rng = ctx.m_rng;
    const auto& binary_vars = ctx.m_shared.m_binary_idx_list;
    const auto& best_values = ctx.m_shared.m_var_best_value;

    // Use user_data for counting
    if (p_user_data)
    {
      int* counter = static_cast<int*>(p_user_data);
      (*counter)++;
      printf("=== Restart #%d ===\n", *counter);
    }

    // 1. Reset all constraint weights to initial value
    for (size_t i = 0; i < weights.size(); ++i)
    {
      weights[i] = 1;
    }

    // 2. Start from best solution
    if (ctx.m_shared.m_is_found_feasible)
      for (size_t i = 0; i < values.size(); ++i)
      {
        values[i] = best_values[i];
      }

    // 3. Randomly flip 20% of binary variables for perturbation
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    size_t flipped_count = 0;

    for (size_t var_idx : binary_vars)
    {
      if (prob_dist(rng) < 0.2) // 20% flip probability
      {
        values[var_idx] = (values[var_idx] > 0.5) ? 0.0 : 1.0;
        flipped_count++;
      }
    }

    printf("Restart: Reset weights, flipped %zu/%zu binary variables "
           "(%.1f%%)\n",
           flipped_count,
           binary_vars.size(),
           100.0 * flipped_count / binary_vars.size());
  };

  // Pass user_data pointer
  solver.set_restart_cbk(restart_cbk, &restart_count);
  solver.set_restart_step(5000); // Restart after 5000 steps without improvement
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_restart.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_restart.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  return 0;
}
