/*=====================================================================================

    Filename:     test_neighbor_config.cpp

    Description:  Unit tests for Neighbor configuration API

        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include "local_search/context/context.h"
#include "local_search/neighbor/neighbor.h"
#include "model_data/Model_Manager.h"
#include <cassert>
#include <cstdio>
#include <random>

// Simple custom neighbor for testing
static bool custom_neighbor_called = false;

void test_custom_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  custom_neighbor_called = true;
  (void)p_user_data;  // Unused
  ctx.clear_ops();
}

void test_neighbor_ctx_output_helpers()
{
  Model_Manager model_manager;
  std::vector<double> var_current_value;
  std::vector<double> var_best_value;
  std::vector<double> con_activity;
  std::vector<double> con_constant;
  std::vector<bool> con_is_equality;
  std::vector<size_t> con_weight;
  std::vector<size_t> con_unsat_idxs;
  std::vector<size_t> con_pos_in_unsat_idxs;
  std::vector<size_t> con_sat_idxs;
  std::vector<size_t> var_last_dec_step;
  std::vector<size_t> var_last_inc_step;
  std::vector<size_t> var_allow_inc_step;
  std::vector<size_t> var_allow_dec_step;
  size_t obj_var_num = 0;
  std::vector<double> var_obj_cost;
  bool is_found_feasible = false;
  double best_obj = 0.0;
  size_t cur_step = 0;
  size_t last_improve_step = 0;
  bool current_obj_breakthrough = false;
  std::vector<size_t> binary_idx_list;
  std::vector<size_t> non_fixed_var_idx_list;
  Readonly_Ctx shared(model_manager,
                      var_current_value,
                      var_best_value,
                      con_activity,
                      con_constant,
                      con_is_equality,
                      con_weight,
                      con_unsat_idxs,
                      con_pos_in_unsat_idxs,
                      con_sat_idxs,
                      var_last_dec_step,
                      var_last_inc_step,
                      var_allow_inc_step,
                      var_allow_dec_step,
                      obj_var_num,
                      var_obj_cost,
                      is_found_feasible,
                      best_obj,
                      cur_step,
                      last_improve_step,
                      current_obj_breakthrough,
                      binary_idx_list,
                      non_fixed_var_idx_list);
  std::vector<size_t> op_var_idxs;
  std::vector<double> op_var_deltas;
  size_t op_size = 0;
  std::mt19937 rng(0);
  Neighbor::Neighbor_Ctx ctx(
      shared, op_var_idxs, op_var_deltas, op_size, rng);

  ctx.set_single_op(3, -1.5);
  assert(ctx.m_op_size == 1);
  assert(ctx.m_op_var_idxs.size() == 1);
  assert(ctx.m_op_var_deltas.size() == 1);
  assert(ctx.m_op_var_idxs[0] == 3);
  assert(ctx.m_op_var_deltas[0] == -1.5);

  ctx.append_op(5, 2.0);
  assert(ctx.m_op_size == 2);
  assert(ctx.m_op_var_idxs.size() == 2);
  assert(ctx.m_op_var_deltas.size() == 2);
  assert(ctx.m_op_var_idxs[1] == 5);
  assert(ctx.m_op_var_deltas[1] == 2.0);

  ctx.clear_ops();
  assert(ctx.m_op_size == 0);
  assert(ctx.m_op_var_idxs.empty());
  assert(ctx.m_op_var_deltas.empty());
}

int main()
{
  printf("Start Neighbor configuration API tests...\n\n");

  printf("Test 0: Neighbor_Ctx output helpers\n");
  test_neighbor_ctx_output_helpers();
  printf("  ✓ Helper methods keep op buffers and size consistent\n\n");

  Local_MIP solver;
  solver.set_time_limit(0.1);  // Short time run

  // Test 1: Clear list
  printf("Test 1: clear_neighbor_list()\n");
  solver.clear_neighbor_list();
  printf("  ✓ Successfully cleared neighbor list\n\n");

  // Test 2: Add predefined neighbor
  printf("Test 2: add_neighbor()\n");
  solver.add_neighbor("flip", 0, 10);
  solver.add_neighbor("easy", 0, 5);
  printf("  ✓ Successfully added 2 predefined neighbors\n\n");

  // Test 3: Add custom neighbor
  printf("Test 3: add_custom_neighbor()\n");
  solver.add_custom_neighbor("test_custom", test_custom_neighbor);
  printf("  ✓ Successfully added custom neighbor\n\n");

  // Test 4: Reset to default list
  printf("Test 4: reset_default_neighbor_list()\n");
  solver.reset_default_neighbor_list();
  printf("  ✓ Successfully reset to default neighbor list\n\n");

  // Test 5: Mix use of
  printf("Test 5: Mixed use of predefined and custom neighbors\n");
  solver.clear_neighbor_list();
  solver.add_neighbor("unsat_mtm_bm", 12, 8);
  solver.add_custom_neighbor("test_custom", test_custom_neighbor);
  solver.add_neighbor("flip", 0, 12);
  printf("  ✓ Successfully configured mixed neighbor list\n\n");

  printf("All Neighbor configuration API tests passed!\n");

  return 0;
}
