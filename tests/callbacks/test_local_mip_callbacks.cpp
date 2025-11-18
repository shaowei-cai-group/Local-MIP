/*=====================================================================================

    Filename:     test_local_mip_callbacks.cpp

    Description:  Callback Tests - Tests Start/Restart/Weight/Neighbor callback mechanisms
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sstream>
#include <vector>

#define private public
#define protected public
#include "Local_MIP.h"
#include "local_search/Local_Search.h"
#include "local_search/restart/restart.h"
#include "local_search/scoring/scoring.h"
#include "local_search/start/start.h"
#include "local_search/weight/weight.h"
#undef private
#undef protected

namespace
{

struct SharedData
{
  explicit SharedData(Model_Manager& manager)
      : var_best_value(), con_activity(), con_constant(),
        con_is_equality(), con_weight(), con_unsat_idxs(),
        con_pos_in_unsat_idxs(), var_last_dec_step(), var_last_inc_step(),
        var_obj_cost(), is_found_feasible(false), best_obj(0.0),
        cur_step(0), last_improve_step(0), var_allow_inc_step(),
        var_allow_dec_step(), current_obj_breakthrough(false),
        obj_var_num(0), binary_idx_list(), var_current_value(),
        con_sat_idxs(), non_fixed_var_idxs(),
        view(manager,
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
             non_fixed_var_idxs)
  {
  }

  std::vector<double> var_best_value;
  std::vector<double> con_activity;
  std::vector<double> con_constant;
  std::vector<bool> con_is_equality;
  std::vector<size_t> con_weight;
  std::vector<size_t> con_unsat_idxs;
  std::vector<size_t> con_pos_in_unsat_idxs;
  std::vector<size_t> var_last_dec_step;
  std::vector<size_t> var_last_inc_step;
  std::vector<double> var_obj_cost;
  bool is_found_feasible;
  double best_obj;
  size_t cur_step;
  size_t last_improve_step;
  std::vector<size_t> var_allow_inc_step;
  std::vector<size_t> var_allow_dec_step;
  bool current_obj_breakthrough;
  size_t obj_var_num;
  std::vector<size_t> binary_idx_list;
  std::vector<double> var_current_value;
  std::vector<size_t> con_sat_idxs;
  std::vector<size_t> non_fixed_var_idxs;
  Readonly_Ctx view;
};

bool check(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "ERROR: %s\n", message);
    return false;
  }
  return true;
}

bool test_start_cbk_invocation()
{
  Local_MIP solver;
  bool invoked = false;

  solver.set_start_cbk(
      [&](Start::Start_Ctx& context, void*)
      {
        invoked = true;
        context.m_var_current_value[0] = 42.0;
      });

  SharedData shared(*solver.m_model_manager);
  std::vector<double> var_values(1, 0.0);
  std::mt19937 random_engine(1);
  Start::Start_Ctx context(shared.view, var_values, random_engine);

  solver.m_local_search->m_start.set_up_start_values(context);

  bool ok = true;
  ok &= check(invoked, "start callback should be invoked");
  ok &= check(std::fabs(var_values[0] - 42.0) < 1e-9,
              "start callback should mutate start values");
  return ok;
}

bool test_restart_cbk_invocation()
{
  Local_MIP solver;
  bool invoked = false;
  solver.set_restart_step(1);

  solver.set_restart_cbk(
      [&](Restart::Restart_Ctx& context, void*)
      {
        invoked = true;
        context.m_var_current_value[0] = 99.0;
      });

  SharedData shared(*solver.m_model_manager);
  std::vector<double> current_values(1, 0.0);
  std::mt19937 random_engine(2);
  bool has_feasible = false;
  size_t cur_step = 10;
  size_t last_improve_step = 0;

  shared.con_weight.assign(1, 0);
  shared.var_best_value.assign(1, -5.0);
  shared.is_found_feasible = has_feasible;
  shared.cur_step = cur_step;
  shared.last_improve_step = last_improve_step;

  Restart::Restart_Ctx context(
      shared.view, current_values, random_engine, shared.con_weight);

  bool restarted = solver.m_local_search->m_restart.execute(context);

  bool ok = true;
  ok &= check(restarted, "execute should report that restart happened");
  ok &= check(invoked, "restart callback should be invoked");
  ok &= check(std::fabs(current_values[0] - 99.0) < 1e-9,
              "restart callback should control current values");
  return ok;
}

bool test_weight_cbk_invocation()
{
  Local_MIP solver;
  bool invoked = false;

  solver.set_weight_cbk(
      [&](Weight::Weight_Ctx& context, void*)
      {
        invoked = true;
        context.m_con_weight[0] = 777;
      });

  SharedData shared(*solver.m_model_manager);
  shared.con_weight.assign(1, 0);
  shared.con_activity.assign(1, 0.0);
  shared.con_constant.assign(1, 0.0);
  shared.con_is_equality.assign(1, false);
  std::mt19937 random_engine(3);
  size_t weight_smooth_prob = 0;
  bool is_found_feasible = false;

  solver.set_weight_smooth_probability(weight_smooth_prob);
  shared.is_found_feasible = is_found_feasible;

  Weight::Weight_Ctx context(
      shared.view, random_engine, shared.con_weight);

  solver.m_local_search->m_weight.update(context);

  bool ok = true;
  ok &= check(invoked, "weight callback should be invoked");
  ok &= check(shared.con_weight[0] == 777,
              "weight callback should adjust weights");
  return ok;
}

bool test_lift_scoring_cbk_invocation()
{
  Local_MIP solver;
  bool invoked = false;
  size_t captured_idx = SIZE_MAX;
  double captured_delta = 0.0;

  solver.set_lift_scoring_cbk(
      [&](Scoring::Lift_Ctx& ctx, size_t var_idx, double delta, void* user_data)
      {
        invoked = true;
        captured_idx = var_idx;
        captured_delta = delta;
        ctx.m_best_var_idx = var_idx;
        ctx.m_best_delta = delta;
        ctx.m_best_lift_score = 123.0;
      });

  std::vector<double> var_obj_cost = {1.0};
  std::vector<size_t> last_dec_step = {0};
  std::vector<size_t> last_inc_step = {0};
  double best_lift_score = 0.0;
  size_t current_best_var_idx = SIZE_MAX;
  double current_best_delta = 0.0;
  size_t current_best_age = SIZE_MAX;
  std::mt19937 random_engine(4);

  SharedData shared(*solver.m_model_manager);
  shared.var_obj_cost = var_obj_cost;
  shared.var_last_dec_step = last_dec_step;
  shared.var_last_inc_step = last_inc_step;

  Scoring::Lift_Ctx context(shared.view,
                            random_engine,
                            best_lift_score,
                            current_best_var_idx,
                            current_best_delta,
                            current_best_age);

  solver.m_local_search->m_scoring.score_lift(context, 0, -2.0);

  bool ok = true;
  ok &= check(invoked, "lift scoring callback should be invoked");
  ok &= check(captured_idx == 0,
              "lift scoring callback should receive variable index");
  ok &= check(std::fabs(captured_delta + 2.0) < 1e-9,
              "lift scoring callback should receive delta");
  ok &= check(context.m_best_var_idx == 0,
              "lift scoring callback should be able to set best variable");
  ok &= check(context.m_best_lift_score == 123.0,
              "lift scoring callback should control best score");
  return ok;
}

bool test_neighbor_scoring_cbk_invocation()
{
  Local_MIP solver;
  bool invoked = false;
  size_t captured_idx = SIZE_MAX;
  double captured_delta = 0.0;

  solver.set_neighbor_scoring_cbk(
      [&](Scoring::Neighbor_Ctx& ctx, size_t var_idx, double delta, void* user_data)
      {
        invoked = true;
        captured_idx = var_idx;
        captured_delta = delta;
        ctx.m_best_var_idx = var_idx;
        ctx.m_best_delta = delta;
        ctx.m_best_neighbor_score = 10;
        ctx.m_best_neighbor_subscore = 5;
      });

  std::vector<uint32_t> binary_stamp(1, 0);
  uint32_t binary_stamp_token = 1;
  std::vector<double> con_activity(1, 0.0);
  std::vector<double> con_constant(1, 0.0);
  std::vector<bool> con_is_equality(1, false);
  std::vector<size_t> con_weight(1, 1);
  std::vector<size_t> last_dec_step = {0};
  std::vector<size_t> last_inc_step = {0};
  bool is_found_feasible = false;
  double best_obj = 0.0;
  long current_best_score = -1;
  long current_best_subscore = -1;
  size_t current_best_age = SIZE_MAX;
  size_t current_best_var_idx = SIZE_MAX;
  double current_best_delta = 0.0;

  SharedData shared(*solver.m_model_manager);
  shared.con_activity = con_activity;
  shared.con_constant = con_constant;
  shared.con_is_equality = con_is_equality;
  shared.con_weight = con_weight;
  shared.var_last_dec_step = last_dec_step;
  shared.var_last_inc_step = last_inc_step;
  shared.is_found_feasible = is_found_feasible;
  shared.best_obj = best_obj;

  Scoring::Neighbor_Ctx context(shared.view,
                                binary_stamp,
                                binary_stamp_token,
                                current_best_score,
                                current_best_subscore,
                                current_best_age,
                                current_best_var_idx,
                                current_best_delta);

  solver.m_local_search->m_scoring.score_neighbor(context, 0, 1.5);

  bool ok = true;
  ok &= check(invoked, "neighbor scoring callback should be invoked");
  ok &= check(captured_idx == 0,
              "neighbor scoring callback should receive variable index");
  ok &= check(std::fabs(captured_delta - 1.5) < 1e-9,
              "neighbor scoring callback should receive delta");
  ok &= check(context.m_best_var_idx == 0,
              "neighbor scoring callback should set best variable");
  ok &= check(context.m_best_neighbor_score == 10,
              "neighbor scoring callback should set scores");
  ok &= check(context.m_best_neighbor_subscore == 5,
              "neighbor scoring callback should set sub-scores");
  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_start_cbk_invocation();
  ok &= test_restart_cbk_invocation();
  ok &= test_weight_cbk_invocation();
  ok &= test_lift_scoring_cbk_invocation();
  ok &= test_neighbor_scoring_cbk_invocation();

  if (!ok)
  {
    std::fprintf(stderr, "c Local_MIP callback tests FAILED.\n");
    return EXIT_FAILURE;
  }

  std::printf("c Local_MIP callback tests PASSED.\n");
  return EXIT_SUCCESS;
}
