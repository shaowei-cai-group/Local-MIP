/*=====================================================================================

    Filename:     neighbor_config.cpp

    Description:  Neighbor List Configuration API Example
                  Demonstrates how to customize neighbor list and order, and add custom neighbors

        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include <cstdio>

// Custom Neighbor: randomly flip binary variable
void my_random_flip_neighbor(Neighbor::Neighbor_Ctx& ctx,
                             void* p_user_data)
{
  (void)p_user_data; // Unused

  // Randomly select a binary variable
  const auto& binary_list = ctx.m_shared.m_binary_idx_list;
  if (binary_list.empty())
    return;

  std::uniform_int_distribution<size_t> dist(0, binary_list.size() - 1);
  size_t random_idx = dist(ctx.m_rng);
  size_t var_idx = binary_list[random_idx];

  // Flip the variable (0 to 1, 1 to 0)
  double current_value = ctx.m_shared.m_var_current_value[var_idx];
  double delta = (current_value < 0.5) ? 1.0 : -1.0;

  // Set operation
  ctx.m_op_size = 1;
  ctx.m_op_var_idxs[0] = var_idx;
  ctx.m_op_var_deltas[0] = delta;
}

// Custom Neighbor: gradient descent (only for feasible phase)
void my_gradient_descent_neighbor(Neighbor::Neighbor_Ctx& ctx,
                                  void* p_user_data)
{
  (void)p_user_data; // Unused

  // Only use when feasible solution found
  if (!ctx.m_shared.m_is_found_feasible)
    return;

  // Find variable with largest objective coefficient
  size_t best_var_idx = SIZE_MAX;
  double best_improvement = 0.0;

  for (size_t var_idx : ctx.m_shared.m_non_fixed_var_idx_list)
  {
    double obj_cost = ctx.m_shared.m_var_obj_cost[var_idx];
    if (obj_cost < 0)
    {
      // Negative coefficient, increase variable improves objective
      const auto& model_var = ctx.m_shared.m_model_manager.var(var_idx);
      double current_value = ctx.m_shared.m_var_current_value[var_idx];
      if (current_value < model_var.upper_bound())
      {
        double improvement = -obj_cost;
        if (improvement > best_improvement)
        {
          best_improvement = improvement;
          best_var_idx = var_idx;
        }
      }
    }
    else if (obj_cost > 0)
    {
      // Positive coefficient, decrease variable improves objective
      const auto& model_var = ctx.m_shared.m_model_manager.var(var_idx);
      double current_value = ctx.m_shared.m_var_current_value[var_idx];
      if (current_value > model_var.lower_bound())
      {
        double improvement = obj_cost;
        if (improvement > best_improvement)
        {
          best_improvement = improvement;
          best_var_idx = var_idx;
        }
      }
    }
  }

  if (best_var_idx != SIZE_MAX)
  {
    double obj_cost = ctx.m_shared.m_var_obj_cost[best_var_idx];
    double delta = (obj_cost < 0) ? 1.0 : -1.0;

    ctx.m_op_size = 1;
    ctx.m_op_var_idxs[0] = best_var_idx;
    ctx.m_op_var_deltas[0] = delta;
  }
}

int main(int argc, char** argv)
{
  const char* instance_file =
      (argc > 1) ? argv[1] : "test-set/2club200v15p5scn.mps";

  printf("\n========== Example 1: Use default Neighbor list ==========\n");
  {
    Local_MIP solver;
    solver.set_model_file(instance_file);
    solver.set_time_limit(10.0);
    solver.set_sol_path("example_neighbor_config.sol");
    // default list is：
    // 1. unsat_mtm_bm
    // 2. sat_mtm
    // 3. flip
    // 4. easy
    // 5. unsat_mtm_bm_random
    printf("c Use default list\n");
    solver.run();
  }

  printf("\n========== Example 2: Custom Neighbor order ==========\n");
  {
    Local_MIP solver;
    solver.set_model_file(instance_file);
    solver.set_time_limit(10.0);
    solver.set_sol_path("example_neighbor_config.sol");

    // Clear default list, only use flip and easy
    solver.clear_neighbor_list();
    solver.add_neighbor("flip", 0, 12);
    solver.add_neighbor("easy", 0, 8);

    printf("c Custom list: [flip, easy]\n");
    solver.run();
  }

  printf("\n========== Example 3: Add custom Neighbor ==========\n");
  {
    Local_MIP solver;
    solver.set_model_file(instance_file);
    solver.set_time_limit(10.0);
    solver.set_sol_path("example_neighbor_config.sol");

    // Add custom on top of default list neighbor
    solver.add_custom_neighbor("my_random_flip", my_random_flip_neighbor);
    solver.add_custom_neighbor("my_gradient_descent",
                               my_gradient_descent_neighbor);

    printf("c List: [5 defaults + my_random_flip + my_gradient_descent]\n");
    solver.run();
  }

  printf("\n========== Example 4: Use custom Neighbor only ==========\n");
  {
    Local_MIP solver;
    solver.set_model_file(instance_file);
    solver.set_time_limit(10.0);
    solver.set_sol_path("example_neighbor_config.sol");

    // Clearlist, only use custom neighbor
    solver.clear_neighbor_list();
    solver.add_custom_neighbor("my_random_flip", my_random_flip_neighbor);
    solver.add_custom_neighbor("my_gradient_descent",
                               my_gradient_descent_neighbor);

    printf("c List: [my_random_flip, my_gradient_descent]\n");
    solver.run();
  }

  printf("\n========== Example 5: Mix predefined and custom Neighbor ==========\n");
  {
    Local_MIP solver;
    solver.set_model_file(instance_file);
    solver.set_time_limit(10.0);
    solver.set_sol_path("example_neighbor_config.sol");

    // First add custom, thenAdd predefined
    solver.clear_neighbor_list();
    solver.add_custom_neighbor("my_random_flip", my_random_flip_neighbor);
    solver.add_neighbor("unsat_mtm_bm", 12, 8);
    solver.add_neighbor("flip", 0, 12);
    solver.add_custom_neighbor("my_gradient_descent",
                               my_gradient_descent_neighbor);

    printf("c List: [my_random_flip, unsat_mtm_bm, flip, "
           "my_gradient_descent]\n");
    solver.run();
  }

  return 0;
}
