/*=====================================================================================

    Filename:     start_callback.cpp

    Description:  Start Callback Example - Demonstrates custom initial solution generation via callback
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

  // User-defined data: counter to track callback invocations
  int call_count = 0;

  // Custom start callback: randomly assign 0 or 1 to all binary variables
  // Note: callback now accepts void* p_user_data parameter
  Local_Search::Start_Cbk cbk = [](Start::Start_Ctx& ctx, void* p_user_data)
  {
    auto& values = ctx.m_var_current_value;
    auto& rng = ctx.m_rng;
    const auto& binary_vars = ctx.m_shared.m_binary_idx_list;

    // Use user_data for counting
    if (p_user_data)
    {
      int* counter = static_cast<int*>(p_user_data);
      (*counter)++;
      printf("Callback called %d time(s)\n", *counter);
    }

    // Use uniform distribution to generate 0 or 1
    std::uniform_int_distribution<int> dist(0, 1);

    // Iterate through all binary variables and assign randomly
    for (size_t var_idx : binary_vars)
    {
      values[var_idx] = dist(rng);
    }

    printf("Callback: Randomly initialized %zu binary variables\n",
           binary_vars.size());
  };

  // Pass user_data pointer
  solver.set_start_cbk(cbk, &call_count);
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_callback.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_callback.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  return 0;
}
