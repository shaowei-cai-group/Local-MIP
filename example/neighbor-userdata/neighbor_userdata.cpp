/*=====================================================================================

    Filename:     neighbor_userdata.cpp

    Description:  Demonstrates how to use void* p_user_data to pass custom data to Neighbor callback

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

// User-defined data structure: statistics
struct NeighborStats
{
  size_t total_calls = 0;
  size_t successful_ops = 0;
  size_t failed_ops = 0;
  size_t binary_flips = 0;
  size_t gradient_steps = 0;
};

// Custom Neighbor 1: smart flip (with statistics)
void smart_flip_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  // Get user data
  NeighborStats* stats = static_cast<NeighborStats*>(p_user_data);
  if (stats)
    stats->total_calls++;

  // Prioritizebinary variables with negative objective coefficient
  const auto& binary_list = ctx.m_shared.m_binary_idx_list;
  if (binary_list.empty())
  {
    if (stats)
      stats->failed_ops++;
    return;
  }

  size_t best_var_idx = SIZE_MAX;
  double best_obj_improvement = 0.0;

  for (size_t var_idx : binary_list)
  {
    double current_value = ctx.m_shared.m_var_current_value[var_idx];
    double obj_cost = ctx.m_shared.m_var_obj_cost[var_idx];
    double delta = (current_value < 0.5) ? 1.0 : -1.0;

    // Calculateobjective improvement after flip
    double improvement = -delta * obj_cost;

    if (improvement > best_obj_improvement)
    {
      const auto& var = ctx.m_shared.m_model_manager.var(var_idx);
      if (var.in_bound(current_value + delta))
      {
        best_obj_improvement = improvement;
        best_var_idx = var_idx;
      }
    }
  }

  // Ifno improvement found, select randomly
  if (best_var_idx == SIZE_MAX)
  {
    std::uniform_int_distribution<size_t> dist(0, binary_list.size() - 1);
    best_var_idx = binary_list[dist(ctx.m_rng)];
  }

  double current_value = ctx.m_shared.m_var_current_value[best_var_idx];
  double delta = (current_value < 0.5) ? 1.0 : -1.0;

  // Set operation
  ctx.m_op_size = 1;
  ctx.m_op_var_idxs[0] = best_var_idx;
  ctx.m_op_var_deltas[0] = delta;

  if (stats)
  {
    stats->successful_ops++;
    stats->binary_flips++;
  }
}

// Custom Neighbor 2: greedy gradient descent (with statistics)
void greedy_gradient_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  // Get user data
  NeighborStats* stats = static_cast<NeighborStats*>(p_user_data);
  if (stats)
    stats->total_calls++;

  // Only use when feasible solution found
  if (!ctx.m_shared.m_is_found_feasible)
  {
    if (stats)
      stats->failed_ops++;
    return;
  }

  size_t best_var_idx = SIZE_MAX;
  double best_delta = 0.0;
  double best_improvement = 0.0;

  for (size_t var_idx : ctx.m_shared.m_non_fixed_var_idx_list)
  {
    double obj_cost = ctx.m_shared.m_var_obj_cost[var_idx];
    const auto& model_var = ctx.m_shared.m_model_manager.var(var_idx);
    double current_value = ctx.m_shared.m_var_current_value[var_idx];

    // Try toincrease variable
    if (obj_cost < 0 && current_value < model_var.upper_bound())
    {
      double delta = model_var.is_real() ? 1.0 : 1.0;
      if (model_var.in_bound(current_value + delta))
      {
        double improvement = -obj_cost * delta;
        if (improvement > best_improvement)
        {
          best_improvement = improvement;
          best_var_idx = var_idx;
          best_delta = delta;
        }
      }
    }

    // Try todecrease variable
    if (obj_cost > 0 && current_value > model_var.lower_bound())
    {
      double delta = model_var.is_real() ? -1.0 : -1.0;
      if (model_var.in_bound(current_value + delta))
      {
        double improvement = -obj_cost * delta;
        if (improvement > best_improvement)
        {
          best_improvement = improvement;
          best_var_idx = var_idx;
          best_delta = delta;
        }
      }
    }
  }

  if (best_var_idx != SIZE_MAX)
  {
    ctx.m_op_size = 1;
    ctx.m_op_var_idxs[0] = best_var_idx;
    ctx.m_op_var_deltas[0] = best_delta;

    if (stats)
    {
      stats->successful_ops++;
      stats->gradient_steps++;
    }
  }
  else
  {
    ctx.m_op_size = 0;
    if (stats)
      stats->failed_ops++;
  }
}

int main(int argc, char** argv)
{
  const char* instance_file = (argc > 1) ? argv[1] : "test-set/2club200v15p5scn.mps";

  printf("========== Neighbor User Data Example ==========\n\n");
  printf("Instance: %s\n\n", instance_file);

  Local_MIP solver;
  solver.set_model_file(instance_file);
  solver.set_time_limit(30.0);
  solver.set_sol_path("example_neighbor_userdata.sol");

  // Create statistics structure
  NeighborStats stats;

  // Cleardefault list
  solver.clear_neighbor_list();

  // Addcustom neighbor and pass user_data pointer
  printf("Add custom Neighbors with statistics:\n");
  printf("  - smart_flip: smart flip binary variables\n");
  printf("  - greedy_gradient: greedy gradient descent\n\n");

  solver.add_custom_neighbor("smart_flip", smart_flip_neighbor, &stats);
  solver.add_custom_neighbor("greedy_gradient", greedy_gradient_neighbor, &stats);

  // Can also add predefined neighbor
  solver.add_neighbor("flip", 0, 12);

  printf("Start solving...\n\n");
  solver.run();

  // Output statistics
  printf("\n========== Neighbor Statistics ==========\n");
  printf("Total calls:     %zu\n", stats.total_calls);
  printf("Successful operations:   %zu\n", stats.successful_ops);
  printf("Failed operations:   %zu\n", stats.failed_ops);
  printf("Binary variable flips:   %zu\n", stats.binary_flips);
  printf("Gradient descent steps:   %zu\n", stats.gradient_steps);

  if (stats.total_calls > 0)
  {
    printf("\nSuccess rate: %.2f%%\n",
           100.0 * stats.successful_ops / stats.total_calls);
  }

  printf("\nSolution status: %s\n",
         solver.is_feasible() ? "Feasible" : "Infeasible");
  if (solver.is_feasible())
  {
    printf("Objective: %.6f\n", solver.get_obj_value());
  }

  return 0;
}
