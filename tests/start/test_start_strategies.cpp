/*=====================================================================================

    Filename:     test_start_strategies.cpp

    Description:  Built-in initial solution strategy tests
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "../test_utils.h"
#include "local_search/context/context.h"
#include "local_search/start/start.h"
#include "model_api/Model_Builder.h"
#include "model_data/Model_Manager.h"
#include "utils/global_defs.h"

using namespace test_utils;

namespace
{

std::vector<double> generate_start(const Model_Manager& p_manager,
                                   const std::string& p_method)
{
  std::vector<double> current_value(p_manager.var_num(), 0.0);
  std::vector<double> best_value(p_manager.var_num(), 0.0);
  std::vector<double> con_activity(p_manager.con_num(), 0.0);
  std::vector<double> con_constant(p_manager.con_num(), 0.0);
  std::vector<size_t> con_weight(p_manager.con_num(), 1);
  std::vector<size_t> con_unsat_idxs;
  std::vector<size_t> con_pos_in_unsat_idxs(p_manager.con_num(),
                                            SIZE_MAX);
  std::vector<size_t> con_sat_idxs;
  std::vector<size_t> var_last_dec_step(p_manager.var_num(), 0);
  std::vector<size_t> var_last_inc_step(p_manager.var_num(), 0);
  std::vector<size_t> var_allow_inc_step(p_manager.var_num(), 0);
  std::vector<size_t> var_allow_dec_step(p_manager.var_num(), 0);
  const size_t obj_var_num = p_manager.obj().term_num();
  const bool is_found_feasible = false;
  const double best_obj = 0.0;
  const size_t cur_step = 0;
  const size_t last_improve_step = 0;
  const bool current_obj_breakthrough = false;

  Readonly_Ctx shared(p_manager,
                      current_value,
                      best_value,
                      con_activity,
                      con_constant,
                      p_manager.con_is_equality(),
                      con_weight,
                      con_unsat_idxs,
                      con_pos_in_unsat_idxs,
                      con_sat_idxs,
                      var_last_dec_step,
                      var_last_inc_step,
                      var_allow_inc_step,
                      var_allow_dec_step,
                      obj_var_num,
                      p_manager.var_obj_cost(),
                      is_found_feasible,
                      best_obj,
                      cur_step,
                      last_improve_step,
                      current_obj_breakthrough,
                      p_manager.binary_idx_list(),
                      p_manager.non_fixed_var_idxs());
  std::mt19937 rng(0);
  Start::Start_Ctx start_ctx(shared, current_value, rng);
  Start start;
  start.set_method(p_method);
  start.set_up_start_values(start_ctx, {});
  return current_value;
}

bool test_objective_guided_minimize()
{
  Model_Builder api;
  const int positive = api.add_var("positive", -4.0, 9.0, 3.0);
  const int negative = api.add_var("negative",
                                   -2.0,
                                   8.0,
                                   -2.0,
                                   Var_Type::general_integer);
  const int zero_span = api.add_var("zero_span", -5.0, 5.0);
  const int zero_positive = api.add_var("zero_positive", 2.0, 9.0);
  const int zero_negative = api.add_var("zero_negative", -9.0, -2.0);
  const int infinite_lower =
      api.add_var("infinite_lower", k_neg_inf, -3.0, 1.0);
  const int infinite_upper =
      api.add_var("infinite_upper", 4.0, k_inf, -1.0);
  const int fixed =
      api.add_var("fixed", 7.0, 7.0, -10.0, Var_Type::fixed);
  const int epsilon_positive = api.add_var(
      "epsilon_positive", -6.0, 6.0, k_default_zero_tolerance);
  const int epsilon_negative = api.add_var(
      "epsilon_negative", -6.0, 6.0, -k_default_zero_tolerance);
  const int above_epsilon = api.add_var(
      "above_epsilon", -6.0, 6.0, 2.0 * k_default_zero_tolerance);

  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = api.prepare(options);
  const auto& manager = prepared->model_manager();
  bool ok = true;
  const auto values = generate_start(manager, "objective");

  ok &= check_double(values[positive],
                     -4.0,
                     "Positive objective should select the lower bound");
  ok &= check_double(values[negative],
                     8.0,
                     "Negative objective should select the upper bound");
  ok &= check_double(values[zero_span],
                     0.0,
                     "Zero objective should select zero when available");
  ok &= check_double(values[zero_positive],
                     2.0,
                     "Zero objective should project zero to a positive domain");
  ok &= check_double(values[zero_negative],
                     -2.0,
                     "Zero objective should project zero to a negative domain");
  ok &= check_double(values[infinite_lower],
                     -3.0,
                     "Infinite selected lower bound should fall back to zero projection");
  ok &= check_double(values[infinite_upper],
                     4.0,
                     "Infinite selected upper bound should fall back to zero projection");
  ok &= check_double(values[fixed],
                     7.0,
                     "Fixed variables should always use their fixed value");
  ok &= check_double(values[epsilon_positive],
                     0.0,
                     "Positive coefficient at zero tolerance should be treated as zero");
  ok &= check_double(values[epsilon_negative],
                     0.0,
                     "Negative coefficient at zero tolerance should be treated as zero");
  ok &= check_double(values[above_epsilon],
                     -6.0,
                     "Coefficient above zero tolerance should guide the start");
  return ok;
}

bool test_objective_guided_maximize()
{
  Model_Builder api;
  api.set_sense(Model_Builder::Sense::maximize);
  const int positive = api.add_var("positive", 1.0, 6.0, 2.0);
  const int negative = api.add_var("negative", -7.0, -1.0, -3.0);

  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = api.prepare(options);
  const auto& manager = prepared->model_manager();
  bool ok = true;
  ok &= check_double(manager.var_obj_cost()[positive],
                     -2.0,
                     "Maximization cost should be converted to internal minimization");
  ok &= check_double(manager.var_obj_cost()[negative],
                     3.0,
                     "Negative maximization cost should be negated internally");

  const auto values = generate_start(manager, "objective");
  ok &= check_double(values[positive],
                     6.0,
                     "Maximization positive cost should select the upper bound");
  ok &= check_double(values[negative],
                     -7.0,
                     "Maximization negative cost should select the lower bound");
  return ok;
}

bool test_lock_guided()
{
  Model_Builder api;
  const int only_up = api.add_var("only_up", 1.0, 9.0);
  const int only_down = api.add_var("only_down", -9.0, -1.0);
  const int tied_objective =
      api.add_var("tied_objective", -5.0, 7.0, 2.0);
  const int objective_row_skip =
      api.add_var("objective_row_skip", -6.0, 8.0, 1.0);
  const int equality_zero =
      api.add_var("equality_zero", -5.0, 7.0);
  const int inferred_skip =
      api.add_var("inferred_skip", -4.0, 6.0);
  const int tiny_coeff = api.add_var("tiny_coeff", -3.0, 8.0, -1.0);
  const int fixed =
      api.add_var("fixed", 4.0, 4.0, 0.0, Var_Type::fixed);
  const int infinite_lower =
      api.add_var("infinite_lower", k_neg_inf, -2.0);
  const int infinite_upper =
      api.add_var("infinite_upper", 3.0, k_inf);
  const int equality_objective =
      api.add_var("equality_objective", -8.0, 5.0, -2.0);
  const int tie_guard = api.add_var("tie_guard", -10.0, 10.0);

  api.add_con(k_neg_inf,
              100.0,
              {only_up,
               only_down,
               objective_row_skip,
               inferred_skip,
               tiny_coeff,
               fixed,
               infinite_lower,
               infinite_upper},
              {1.0,
               -1.0,
               -1.0,
               -1.0,
               k_default_zero_tolerance,
               -1.0,
               1.0,
               -1.0});
  api.add_con(k_neg_inf,
              100.0,
              std::vector<int>{tied_objective, tie_guard},
              std::vector<double>{1.0, 1.0});
  api.add_con(k_neg_inf,
              100.0,
              std::vector<int>{tied_objective, tie_guard},
              std::vector<double>{-1.0, -1.0});
  api.add_con(0.0,
              0.0,
              {equality_zero, equality_objective},
              {1.0, -2.0});
  api.add_con(k_neg_inf,
              100.0,
              std::vector<int>{inferred_skip},
              std::vector<double>{1.0});

  Model_Prepare_Options options;
  options.bound_strengthen = 2;
  options.split_eq = false;
  auto prepared = api.prepare(options);
  const auto& manager = prepared->model_manager();
  bool ok = true;
  ok &= check(manager.con(4).is_equality(),
              "Equality should remain unsplit for the lock test");
  ok &= check(manager.con(5).is_inferred_sat(),
              "Singleton presolve should infer the redundant lock row");

  const auto values = generate_start(manager, "locks");
  ok &= check_double(values[only_up],
                     1.0,
                     "Only an up-lock should select the lower bound");
  ok &= check_double(values[only_down],
                     -1.0,
                     "Only a down-lock should select the upper bound");
  ok &= check_double(values[tied_objective],
                     -5.0,
                     "Equal lock counts should use objective guidance");
  ok &= check_double(values[objective_row_skip],
                     8.0,
                     "The objective row should not contribute locks");
  ok &= check_double(values[equality_zero],
                     0.0,
                     "Unsplit equality should add both locks and fall back to zero");
  ok &= check_double(values[inferred_skip],
                     6.0,
                     "Inferred-satisfied constraints should not contribute locks");
  ok &= check_double(values[tiny_coeff],
                     8.0,
                     "Coefficient at zero tolerance should not contribute a lock");
  ok &= check_double(values[fixed],
                     4.0,
                     "Fixed variable should ignore lock direction");
  ok &= check_double(values[infinite_lower],
                     -2.0,
                     "Infinite lock-selected lower bound should fall back to zero projection");
  ok &= check_double(values[infinite_upper],
                     3.0,
                     "Infinite lock-selected upper bound should fall back to zero projection");
  ok &= check_double(values[equality_objective],
                     5.0,
                     "Equality lock tie should use negative objective guidance");
  return ok;
}

bool test_split_equality_locks()
{
  Model_Builder api;
  const int variable = api.add_var("x", -4.0, 9.0);
  api.add_con(0.0,
              0.0,
              std::vector<int>{variable},
              std::vector<double>{1.0});

  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = api.prepare(options);
  const auto& manager = prepared->model_manager();
  bool ok = true;
  ok &= check(manager.con_num() == 3,
              "Split equality should create two inequality rows");
  ok &= check(!manager.con(1).is_equality() &&
                  !manager.con(2).is_equality(),
              "Split equality rows should be counted as inequalities");

  const auto values = generate_start(manager, "locks");
  ok &= check_double(values[variable],
                     0.0,
                     "Opposite split rows should produce an equal lock count");
  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_objective_guided_minimize();
  ok &= test_objective_guided_maximize();
  ok &= test_lock_guided();
  ok &= test_split_equality_locks();

  if (!ok)
  {
    std::fprintf(stderr, "c Local_MIP start strategy tests FAILED.\n");
    return EXIT_FAILURE;
  }
  std::printf("c Local_MIP start strategy tests PASSED.\n");
  return EXIT_SUCCESS;
}
