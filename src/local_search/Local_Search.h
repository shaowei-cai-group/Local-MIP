/*=====================================================================================

    Filename:     Local_Search.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../model_data/Model_Con.h"
#include "../model_data/Model_Manager.h"
#include "../model_data/Model_Var.h"
#include "../utils/global_defs.h"
#include "context/context.h"
#include "neighbor/neighbor.h"
#include "restart/restart.h"
#include "scoring/scoring.h"
#include "start/start.h"
#include "weight/weight.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class Local_Search
{
private:
  const Model_Manager* m_model_manager;

  const std::vector<bool>& m_con_is_equality;

  const std::vector<double>& m_var_obj_cost;

  std::vector<double> m_var_current_value;

  std::vector<double> m_var_best_value;

  std::vector<size_t> m_var_allow_inc_step;

  std::vector<size_t> m_var_allow_dec_step;

  std::vector<size_t> m_var_last_inc_step;

  std::vector<size_t> m_var_last_dec_step;

  std::vector<double> m_var_LB_feas_delta;

  std::vector<double> m_var_UB_feas_delta;

  std::vector<double> m_var_lift_delta;

  std::unordered_set<size_t> m_feas_touch_vars;

  bool m_is_keep_feas;

  bool m_strct_feas;

  bool m_break_eq_feas;

  std::vector<double> m_op_var_deltas;

  std::vector<size_t> m_op_var_idxs;

  size_t m_op_size;

  std::vector<uint32_t> m_binary_op_stamp;

  uint32_t m_binary_op_stamp_token;

  std::vector<size_t> m_con_weight;

  std::vector<double> m_con_activity;

  std::vector<double> m_con_constant;

  std::vector<size_t> m_con_unsat_idxs;

  std::vector<size_t> m_con_pos_in_unsat_idxs;

  std::vector<size_t> m_con_sat_idxs;

  std::vector<size_t> m_con_pos_in_sat_idxs;

  size_t m_activity_period;

  size_t m_activity_hits;

  size_t m_cur_step;

  std::mt19937 m_rng;

  size_t m_tabu_base;

  size_t m_tabu_variation;

  bool m_is_found_feasible;

  bool m_current_obj_breakthrough;

  size_t m_last_improve_step;

  size_t m_bms_unsat_con;

  size_t m_bms_mtm_unsat_op;

  size_t m_bms_sat_con;

  size_t m_bms_mtm_sat_op;

  size_t m_bms_flip_op;

  size_t m_bms_easy_op;

  size_t m_bms_random_op;

  double m_best_obj;

  std::atomic<double> m_logged_obj_value;

  bool m_terminated;

  std::string m_sol_path;

  size_t m_min_unsat_con;

  size_t m_var_num;

  size_t m_con_num;

  size_t m_obj_var_num;

  bool m_has_objective;

  bool m_is_unbounded;

  double m_best_lift_score;

  long m_best_neighbor_score;

  long m_best_neighbor_subscore;

  size_t m_best_age;

  size_t m_best_var_idx;

  double m_best_delta;

  Readonly_Ctx m_readonly_ctx;

  Start::Start_Ctx m_start_ctx;

  Restart::Restart_Ctx m_restart_ctx;

  Weight::Weight_Ctx m_weight_ctx;

  Scoring::Lift_Ctx m_lift_ctx;

  Scoring::Neighbor_Ctx m_scoring_ctx;

  Neighbor::Neighbor_Ctx m_neighbor_ctx;

  Start m_start;

  Restart m_restart;

  Weight m_weight;

  Scoring m_scoring;

  std::vector<Neighbor> m_explore_neighbor_list;

  inline bool con_sat(size_t p_con_idx) const;

  inline bool con_unsat(size_t p_con_idx) const;

  inline void insert_unsat(size_t p_con_idx);

  inline void delete_unsat(size_t p_con_idx);

  inline void insert_sat(size_t p_con_idx);

  inline void delete_sat(size_t p_con_idx);

  inline bool tabu(size_t p_var_idx, double p_delta);

  inline bool tabu_latest(size_t p_var_idx, double p_delta);

  double lift_move_operation(size_t p_term_idx, size_t p_var_idx);

  inline void update_best_solution();

  inline void publish_best_obj();

  bool verify_solution() const;

  bool solve_objective_only();

  void init_state();

  void refresh_activities();

  inline void reset_op(bool p_require_positive);

  bool lift_move();

  void explore_neighbor(std::vector<Neighbor>& p_explore_neighbors);

  void apply_move(size_t p_var_idx, double p_delta);

  void reset_after_restart();

  void init_data();

public:
  using Start_Cbk = Start::Start_Cbk;

  using Restart_Cbk = Restart::Restart_Cbk;

  using Weight_Cbk = Weight::Weight_Cbk;

  using Lift_Scoring_Cbk = Scoring::Lift_Cbk;

  using Neighbor_Scoring_Cbk = Scoring::Neighbor_Cbk;

  using Neighbor_Cbk = Neighbor::Neighbor_Cbk;

  Local_Search(const Model_Manager* p_model_manager);

  ~Local_Search();

  int run_search();

  void output_result() const;

  void write_sol() const;

  inline double get_obj_value() const;

  inline bool is_feasible() const;

  void terminate();

  void set_sol_path(const std::string& p_sol_path);

  void set_random_seed(uint32_t p_seed);

  void set_start_cbk(Start_Cbk p_start_cbk, void* p_user_data = nullptr);

  void set_start_method(const std::string& p_start_name);

  void set_restart_cbk(Restart_Cbk p_restart_cbk,
                       void* p_user_data = nullptr);

  void set_restart_method(const std::string& p_restart_name);

  void set_restart_step(size_t p_restart_step);

  void set_weight_cbk(Weight_Cbk p_weight_cbk,
                      void* p_user_data = nullptr);

  void set_weight_method(const std::string& p_method_name);

  void set_weight_smooth_probability(size_t p_weight_smooth_prob);

  void set_lift_scoring_method(const std::string& p_method_name);

  void set_neighbor_scoring_method(const std::string& p_method_name);

  void set_lift_scoring_cbk(Lift_Scoring_Cbk p_cbk,
                            void* p_user_data = nullptr);

  void set_neighbor_scoring_cbk(Neighbor_Scoring_Cbk p_cbk,
                                void* p_user_data = nullptr);

  void set_bms_unsat_con(size_t p_value);

  void set_bms_mtm_unsat_op(size_t p_value);

  void set_bms_sat_con(size_t p_value);

  void set_bms_mtm_sat_op(size_t p_value);

  void set_bms_flip_op(size_t p_value);

  void set_bms_easy_op(size_t p_value);

  void set_bms_random_op(size_t p_value);

  void clear_neighbor_list();

  void add_neighbor(const std::string& p_neighbor_name,
                    size_t p_bms_con,
                    size_t p_bms_op);

  void add_custom_neighbor(const std::string& p_neighbor_name,
                           Neighbor_Cbk p_neighbor_cbk,
                           void* p_user_data = nullptr);

  void reset_default_neighbor_list();

  void set_tabu_base(size_t p_value);

  void set_activity_period(size_t p_value);

  void set_tabu_variation(size_t p_value);

  void set_break_eq_feas(bool p_break_eq_feas);
};

inline bool Local_Search::con_sat(size_t p_con_idx) const
{
  double gap = m_con_activity[p_con_idx] - m_con_constant[p_con_idx];
  if (m_con_is_equality[p_con_idx])
    return std::fabs(gap) <= k_feas_tolerance;
  else
    return gap <= k_feas_tolerance;
}

inline bool Local_Search::con_unsat(size_t p_con_idx) const
{
  double gap = m_con_activity[p_con_idx] - m_con_constant[p_con_idx];
  if (m_con_is_equality[p_con_idx])
    return std::fabs(gap) > k_feas_tolerance;
  else
    return gap > k_feas_tolerance;
}

inline void Local_Search::insert_unsat(size_t p_con_idx)
{
  assert(m_con_pos_in_unsat_idxs[p_con_idx] == SIZE_MAX);
  m_con_pos_in_unsat_idxs[p_con_idx] = m_con_unsat_idxs.size();
  m_con_unsat_idxs.push_back(p_con_idx);
}

inline void Local_Search::delete_unsat(size_t p_con_idx)
{
  assert(m_con_pos_in_unsat_idxs[p_con_idx] != SIZE_MAX);
  size_t pos = m_con_pos_in_unsat_idxs[p_con_idx];
  size_t last_con_idx = m_con_unsat_idxs.back();
  m_con_unsat_idxs[pos] = last_con_idx;
  m_con_pos_in_unsat_idxs[last_con_idx] = pos;
  m_con_unsat_idxs.pop_back();
  m_con_pos_in_unsat_idxs[p_con_idx] = SIZE_MAX;
}

inline void Local_Search::insert_sat(size_t p_con_idx)
{
  assert(m_con_pos_in_sat_idxs[p_con_idx] == SIZE_MAX);
  m_con_pos_in_sat_idxs[p_con_idx] = m_con_sat_idxs.size();
  m_con_sat_idxs.push_back(p_con_idx);
}

inline void Local_Search::delete_sat(size_t p_con_idx)
{
  assert(m_con_pos_in_sat_idxs[p_con_idx] != SIZE_MAX);
  size_t pos = m_con_pos_in_sat_idxs[p_con_idx];
  size_t last_con_idx = m_con_sat_idxs.back();
  m_con_sat_idxs[pos] = last_con_idx;
  m_con_pos_in_sat_idxs[last_con_idx] = pos;
  m_con_sat_idxs.pop_back();
  m_con_pos_in_sat_idxs[p_con_idx] = SIZE_MAX;
}

inline void Local_Search::update_best_solution()
{
  assert(m_var_best_value.size() == m_var_num);
  assert(m_var_current_value.size() == m_var_num);
  m_last_improve_step = m_cur_step;
  memcpy(m_var_best_value.data(),
         m_var_current_value.data(),
         m_var_num * sizeof(double));
  m_best_obj = m_con_activity[0];
  m_con_constant[0] = m_best_obj - k_opt_tolerance;
  m_current_obj_breakthrough = false;
  publish_best_obj();
}

inline void Local_Search::publish_best_obj()
{
  m_logged_obj_value.store(
      m_model_manager->is_min() *
          (m_best_obj + m_model_manager->obj_offset()),
      std::memory_order_relaxed);
}

inline double Local_Search::get_obj_value() const
{
  return m_logged_obj_value.load(std::memory_order_relaxed);
}

inline bool Local_Search::is_feasible() const
{
  return m_is_found_feasible;
}

inline void Local_Search::reset_op(bool p_require_positive)
{
  ++m_binary_op_stamp_token;
  if (m_binary_op_stamp_token == 0)
  {
    std::fill(m_binary_op_stamp.begin(), m_binary_op_stamp.end(), 0);
    m_binary_op_stamp_token = 1;
  }
  m_best_lift_score =
      p_require_positive ? 0 : std::numeric_limits<double>::lowest();
  m_best_neighbor_score =
      p_require_positive ? 0 : std::numeric_limits<long>::min();
  m_best_neighbor_subscore = std::numeric_limits<long>::min();
  m_best_var_idx = SIZE_MAX;
  m_best_delta = 0;
  m_best_age = SIZE_MAX;
}

inline void
Local_Search::explore_neighbor(std::vector<Neighbor>& p_explore_neighbors)
{
  assert(!p_explore_neighbors.empty());
  reset_op(true);
  for (auto& neighbor : p_explore_neighbors)
  {
    m_op_var_deltas.clear();
    m_op_var_idxs.clear();
    m_op_size = 0;
    if (&neighbor == &p_explore_neighbors.back())
    {
      reset_op(false);
      m_weight.update(m_weight_ctx);
    }
    neighbor.explore(m_neighbor_ctx);
    for (size_t op_idx = 0; op_idx < m_op_size; ++op_idx)
    {
      m_scoring.score_neighbor(
          m_scoring_ctx, m_op_var_idxs[op_idx], m_op_var_deltas[op_idx]);
    }
    if (m_best_neighbor_score > 0)
      break;
  }
  return;
}
