/*=====================================================================================

    Filename:     Local_Search.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../model_data/Model_Con.h"
#include "../model_data/Model_Var.h"
#include "../utils/global_defs.h"
#include "../utils/solver_error.h"
#include "Local_Search.h"
#include "neighbor/neighbor.h"
#include "restart/restart.h"
#include "start/start.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr uint64_t k_max_exact_binary64_integer = uint64_t{1} << 53;

bool exact_binary64_integer(double p_value, int64_t& p_integer)
{
  const double limit = static_cast<double>(k_max_exact_binary64_integer);
  if (!std::isfinite(p_value) || p_value < -limit || p_value > limit ||
      std::trunc(p_value) != p_value)
    return false;
  p_integer = static_cast<int64_t>(p_value);
  return static_cast<double>(p_integer) == p_value;
}

uint64_t integer_magnitude(int64_t p_value)
{
  return p_value < 0 ? static_cast<uint64_t>(-p_value)
                     : static_cast<uint64_t>(p_value);
}

bool exact_bound_product(uint64_t p_lhs,
                         uint64_t p_rhs,
                         uint64_t& p_product)
{
  if (p_lhs != 0 && p_rhs > k_max_exact_binary64_integer / p_lhs)
    return false;
  p_product = p_lhs * p_rhs;
  return true;
}

} // namespace

int Local_Search::run_search(const std::vector<double>& p_start_solution,
                             const std::vector<char>& p_start_mask)
{
  init_data();
  if (solve_objective_only())
    return 0;
  m_start.set_up_start_values(m_start_ctx, p_start_solution, p_start_mask);
  normalize_domain_values(m_var_current_value, "initial solution");
  init_state();

  while (!m_terminated.load(std::memory_order_relaxed))
  {
    if (m_restart.execute(m_restart_ctx))
    {
      if (m_restart.has_user_callback())
        normalize_domain_values(m_var_current_value, "restart solution");
      reset_after_restart();
    }
    if (m_con_unsat_idxs.empty())
    {
      if (m_activity_dirty)
      {
        refresh_activities();
        if (!m_con_unsat_idxs.empty())
        {
          m_is_keep_feas = false;
          continue;
        }
      }
      if (!m_is_found_feasible || m_current_obj_breakthrough)
      {
        update_best_solution();
        m_is_found_feasible = true;
        if (!m_has_objective)
          return 0;
      }
      bool lift_move_successful = lift_move();
      ++m_cur_step;
      if (lift_move_successful)
        continue;
    }
    const bool validate_selected_move =
        explore_neighbor(m_explore_neighbor_list);
    if (validate_selected_move)
      apply_checked_move(
          m_best_var_idx, m_best_delta, "selected neighbor move");
    else
      apply_move(m_best_var_idx, m_best_delta);
    m_is_keep_feas = false;
    ++m_cur_step;
  }
  return 0;
}

bool Local_Search::finalize_result()
{
  if (m_is_unbounded || !m_is_found_feasible || verify_solution())
    return true;
  m_is_found_feasible = false;
  m_logged_obj_value.store(std::numeric_limits<double>::quiet_NaN(),
                           std::memory_order_relaxed);
  return false;
}

void Local_Search::output_result() const
{
  if (m_is_unbounded)
  {
    printf("o problem is unbounded.\n");
    printf("o best objective: %.17g\n", get_obj_value());
    return;
  }
  if (!m_is_found_feasible)
  {
    printf("o no feasible solution found.\n");
    printf("c min unsat constraints: %zu\n", m_min_unsat_con);
  }
  else
  {
    printf("o best objective: %.17g\n", get_obj_value());
    if (m_sol_path != "")
      write_sol();
  }
}

template <typename Accumulator>
Accumulator
Local_Search::compute_activity(const Model_Con& p_con,
                               const double* p_var_values) const
{
  Accumulator activity = static_cast<Accumulator>(0.0);
  const auto& coeffs = p_con.coeff_set();
  const auto& var_idxs = p_con.var_idx_set();
  for (size_t term_idx = 0; term_idx < coeffs.size(); ++term_idx)
  {
    activity += static_cast<Accumulator>(coeffs[term_idx]) *
                static_cast<Accumulator>(p_var_values[var_idxs[term_idx]]);
  }
  return activity;
}

template <typename Accumulator>
void Local_Search::refresh_activities_impl()
{
  m_con_unsat_idxs.clear();
  m_con_sat_idxs.clear();
  std::fill(m_con_pos_in_unsat_idxs.begin(),
            m_con_pos_in_unsat_idxs.end(),
            SIZE_MAX);
  std::fill(m_con_pos_in_sat_idxs.begin(),
            m_con_pos_in_sat_idxs.end(),
            SIZE_MAX);
  const auto& model_obj = m_model_manager->obj();
  const double* var_values = m_var_current_value.data();
  Accumulator activity =
      compute_activity<Accumulator>(model_obj, var_values);
  m_current_obj_breakthrough =
      activity <= static_cast<Accumulator>(m_con_constant[0]);
  m_con_activity[0] = static_cast<double>(activity);
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    const auto& model_con = m_model_manager->con(con_idx);
    activity = compute_activity<Accumulator>(model_con, var_values);
    const bool is_sat = con_sat(con_idx, activity);
    m_con_activity[con_idx] = static_cast<double>(activity);
    if (is_sat)
      insert_sat(con_idx);
    else
      insert_unsat(con_idx);
  }
  m_activity_hits = 0;
  m_activity_dirty = false;
}

void Local_Search::refresh_activities()
{
  if (m_use_exact_double_activity)
    refresh_activities_impl<double>();
  else
    refresh_activities_impl<long double>();
}

void Local_Search::init_state()
{
  m_con_constant[0] = k_inf;
  refresh_activities();
}

void Local_Search::reset_after_restart()
{
  std::fill(m_var_allow_inc_step.begin(), m_var_allow_inc_step.end(), 0);
  std::fill(m_var_allow_dec_step.begin(), m_var_allow_dec_step.end(), 0);
  std::fill(m_var_last_inc_step.begin(), m_var_last_inc_step.end(), 0);
  std::fill(m_var_last_dec_step.begin(), m_var_last_dec_step.end(), 0);
  m_last_improve_step = m_cur_step;
  refresh_activities();
}

template <typename Accumulator>
void Local_Search::update_affected_activities(const Model_Var& p_model_var,
                                              double p_delta)
{
  for (size_t term_idx = 0; term_idx < p_model_var.term_num(); ++term_idx)
  {
    const size_t con_idx = p_model_var.con_idx(term_idx);
    const auto& model_con = m_model_manager->con(con_idx);
    const size_t pos_in_con = p_model_var.pos_in_con(term_idx);
    const double coeff = model_con.coeff(pos_in_con);
    const bool maintain_status = (con_idx != 0);
    bool was_sat = false;
    if (maintain_status)
    {
      was_sat = m_con_pos_in_sat_idxs[con_idx] != SIZE_MAX;
      assert(was_sat != (m_con_pos_in_unsat_idxs[con_idx] != SIZE_MAX));
    }
    const Accumulator updated_activity =
        static_cast<Accumulator>(m_con_activity[con_idx]) +
        static_cast<Accumulator>(coeff) *
            static_cast<Accumulator>(p_delta);
    const bool now_sat =
        maintain_status ? con_sat(con_idx, updated_activity) : false;
    if (con_idx == 0)
    {
      m_current_obj_breakthrough =
          updated_activity <= static_cast<Accumulator>(m_con_constant[0]);
    }
    m_con_activity[con_idx] = static_cast<double>(updated_activity);
    if (maintain_status)
    {
      if (was_sat && !now_sat)
      {
        delete_sat(con_idx);
        insert_unsat(con_idx);
      }
      else if (!was_sat && now_sat)
      {
        insert_sat(con_idx);
        delete_unsat(con_idx);
      }
    }
  }
}

void Local_Search::apply_move(size_t p_var_idx, double p_delta)
{
  if (p_var_idx == SIZE_MAX || p_delta == 0)
    return;
  assert(p_var_idx < m_var_num);
  auto& model_var = m_model_manager->var(p_var_idx);
  if (!m_model_manager->var_in_bound(
          model_var, m_var_current_value[p_var_idx] + p_delta))
  {
    p_delta = std::clamp(
        p_delta,
        model_var.lower_bound() - m_var_current_value[p_var_idx],
        model_var.upper_bound() - m_var_current_value[p_var_idx]);
  }
  m_var_current_value[p_var_idx] += p_delta;
  m_activity_dirty = true;
  if (m_use_exact_double_activity)
    update_affected_activities<double>(model_var, p_delta);
  else
  {
    update_affected_activities<long double>(model_var, p_delta);
    ++m_activity_hits;
    if (m_activity_hits >= m_activity_period)
      refresh_activities();
  }
  assert(m_tabu_variation > 0);
  std::uniform_int_distribution<size_t> dist(0, m_tabu_variation - 1);
  if (p_delta > 0)
  {
    m_var_last_inc_step[p_var_idx] = m_cur_step;
    m_var_allow_dec_step[p_var_idx] =
        m_cur_step + m_tabu_base + dist(m_rng);
  }
  else
  {
    m_var_last_dec_step[p_var_idx] = m_cur_step;
    m_var_allow_inc_step[p_var_idx] =
        m_cur_step + m_tabu_base + dist(m_rng);
  }
  if (m_con_unsat_idxs.size() < m_min_unsat_con)
    m_min_unsat_con = m_con_unsat_idxs.size();
  assert(m_model_manager->var_in_bound(model_var,
                                       m_var_current_value[p_var_idx]));
}

double Local_Search::checked_move_value(size_t p_var_idx,
                                        double p_delta,
                                        const char* p_source) const
{
  if (p_var_idx >= m_var_num)
    throw Solver_Error(
        std::string(p_source) +
        " variable index is out of range: " + std::to_string(p_var_idx));
  const auto& model_var = m_model_manager->var(p_var_idx);
  if (!std::isfinite(p_delta))
    throw Solver_Error(std::string(p_source) +
                       " delta is not finite for variable '" +
                       model_var.name() + "'");
  const double old_value = m_var_current_value[p_var_idx];
  const double candidate_value = old_value + p_delta;
  if (!std::isfinite(candidate_value))
    throw Solver_Error(std::string(p_source) +
                       " result is not finite for variable '" +
                       model_var.name() + "'");
  double new_value = candidate_value;
  if (!m_model_manager->normalize_var_value(model_var, new_value))
  {
    std::ostringstream oss;
    oss << p_source << " violates variable domain for '"
        << model_var.name() << "': current=" << old_value
        << ", delta=" << p_delta << ", candidate=" << candidate_value;
    throw Solver_Error(oss.str());
  }
  return new_value;
}

void Local_Search::apply_checked_move(size_t p_var_idx,
                                      double p_delta,
                                      const char* p_source)
{
  if (p_var_idx == SIZE_MAX || p_delta == 0)
    return;
  const double new_value =
      checked_move_value(p_var_idx, p_delta, p_source);
  const double old_value = m_var_current_value[p_var_idx];
  apply_move(p_var_idx, new_value - old_value);
}

bool Local_Search::verify_solution() const
{
  if (!verify_domain_values(m_var_best_value))
    return false;
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    const auto& model_con = m_model_manager->con(con_idx);
    const long double activity =
        compute_activity<long double>(model_con, m_var_best_value.data());
    if (con_sat(con_idx, activity))
      continue;
    printf(m_con_is_equality[con_idx]
               ? "c %s activity[%.15g] = constant[%.15g]\n"
               : "c %s activity[%.15g] < constant[%.15g]\n",
           model_con.name().c_str(),
           static_cast<double>(activity),
           m_con_constant[con_idx]);
    return false;
  }
  const auto& model_obj = m_model_manager->obj();
  const long double obj_value =
      compute_activity<long double>(model_obj, m_var_best_value.data());
  if (std::fabs(static_cast<double>(obj_value) - m_best_obj) >
      m_readonly_ctx.m_opt_tolerance)
  {
    printf("c obj_value[%.15g] = best_obj[%.15g]\n",
           static_cast<double>(obj_value) + m_model_manager->obj_offset(),
           m_best_obj + m_model_manager->obj_offset());
    return false;
  }
  return true;
}

void Local_Search::write_sol() const
{
  if (!m_is_found_feasible || !verify_solution())
  {
    printf("o refusing to write an invalid solution.\n");
    return;
  }
  printf("c best-found solution is written to %s\n", m_sol_path.c_str());
  FILE* sol_file = fopen(m_sol_path.c_str(), "w");
  if (sol_file == nullptr)
  {
    printf("o cannot open solution file %s.\n", m_sol_path.c_str());
    return;
  }
  fprintf(
      sol_file, "%-50s        %s\n", "Variable name", "Variable value");
  for (size_t var_idx = 0; var_idx < m_var_num; var_idx++)
  {
    const auto& model_var = m_model_manager->var(var_idx);
    if (m_var_best_value[var_idx])
      fprintf(sol_file,
              "%-50s        %.*g\n",
              model_var.name().c_str(),
              std::numeric_limits<double>::max_digits10,
              m_var_best_value[var_idx]);
  }
  fclose(sol_file);
}

bool Local_Search::can_use_exact_double_activity() const
{
  if (!std::numeric_limits<double>::is_iec559 ||
      std::numeric_limits<double>::radix != 2 ||
      std::numeric_limits<double>::digits != 53 ||
      std::numeric_limits<double>::max_exponent != 1024 ||
      std::numeric_limits<uint64_t>::digits < 54)
    return false;

  std::vector<bool> relevant_vars(m_var_num, false);
  for (size_t con_idx = 0; con_idx < m_con_num; ++con_idx)
  {
    const auto& model_con = m_model_manager->con(con_idx);
    for (size_t term_idx = 0; term_idx < model_con.term_num(); ++term_idx)
    {
      int64_t coeff_integer = 0;
      if (!exact_binary64_integer(model_con.coeff(term_idx),
                                  coeff_integer))
        return false;
      if (coeff_integer == 0)
        continue;
      const size_t var_idx = model_con.var_idx(term_idx);
      if (var_idx >= m_var_num)
        return false;
      relevant_vars[var_idx] = true;
    }
  }

  std::vector<uint64_t> max_abs_values(m_var_num, 0);
  std::vector<uint64_t> domain_widths(m_var_num, 0);
  for (size_t var_idx = 0; var_idx < m_var_num; ++var_idx)
  {
    if (!relevant_vars[var_idx])
      continue;
    const auto& model_var = m_model_manager->var(var_idx);
    int64_t lower_integer = 0;
    int64_t upper_integer = 0;
    if (!exact_binary64_integer(model_var.lower_bound(), lower_integer) ||
        !exact_binary64_integer(model_var.upper_bound(), upper_integer))
      return false;
    if (lower_integer > upper_integer)
      return false;
    if (lower_integer != upper_integer &&
        !model_var.requires_integrality())
      return false;
    const uint64_t width =
        static_cast<uint64_t>(upper_integer - lower_integer);
    if (width > k_max_exact_binary64_integer)
      return false;
    domain_widths[var_idx] = width;
    max_abs_values[var_idx] = std::max(integer_magnitude(lower_integer),
                                       integer_magnitude(upper_integer));
  }

  for (size_t con_idx = 0; con_idx < m_con_num; ++con_idx)
  {
    const auto& model_con = m_model_manager->con(con_idx);
    uint64_t row_bound = 0;
    for (size_t term_idx = 0; term_idx < model_con.term_num(); ++term_idx)
    {
      int64_t coeff_integer = 0;
      if (!exact_binary64_integer(model_con.coeff(term_idx),
                                  coeff_integer))
        return false;
      const uint64_t abs_coeff = integer_magnitude(coeff_integer);
      if (abs_coeff == 0)
        continue;
      const size_t var_idx = model_con.var_idx(term_idx);
      uint64_t term_bound = 0;
      if (!exact_bound_product(
              abs_coeff, max_abs_values[var_idx], term_bound) ||
          term_bound > k_max_exact_binary64_integer - row_bound)
        return false;
      row_bound += term_bound;
      if (domain_widths[var_idx] >
          k_max_exact_binary64_integer / abs_coeff)
        return false;
    }
  }

  return true;
}

void Local_Search::configure_activity_arithmetic()
{
  m_use_exact_double_activity = can_use_exact_double_activity();
  if (m_use_exact_double_activity)
  {
    printf("c activity arithmetic: exact double (certified)\n");
    printf("c periodic activity recomputation: disabled (T=infinity)\n");
  }
  else
    printf("c activity arithmetic: long double\n");
}

void Local_Search::init_data()
{
  assert(m_model_manager->con_num() > 0);
  assert(m_model_manager->var_num() > 0);
  m_min_unsat_con = m_model_manager->con_num();
  m_var_num = m_model_manager->var_num();
  m_obj_var_num = m_model_manager->obj().term_num();
  m_con_num = m_model_manager->con_num();
  m_has_objective = (m_obj_var_num > 0);
  m_is_unbounded = false;
  m_activity_period =
      std::max<size_t>(m_activity_period, static_cast<size_t>(1));
  m_activity_hits = 0;
  m_activity_dirty = false;
  configure_activity_arithmetic();
  m_var_current_value.resize(m_var_num, 0.0);
  m_var_best_value.resize(m_var_num, 0.0);
  m_var_allow_inc_step.resize(m_var_num, 0);
  m_var_allow_dec_step.resize(m_var_num, 0);
  m_var_last_inc_step.resize(m_var_num, 0);
  m_var_last_dec_step.resize(m_var_num, 0);
  m_op_var_deltas.reserve(m_var_num);
  m_op_var_idxs.reserve(m_var_num);
  m_feas_touch_vars.reserve(m_var_num);
  m_binary_op_stamp.assign(m_var_num, 0);
  m_binary_op_stamp_token = 0;
  m_var_LB_feas_delta.resize(m_obj_var_num, 0.0);
  m_var_UB_feas_delta.resize(m_obj_var_num, 0.0);
  m_var_lift_delta.resize(m_obj_var_num, 0.0);
  m_con_weight.resize(m_con_num, 1);
  m_con_pos_in_unsat_idxs.resize(m_con_num, SIZE_MAX);
  m_con_pos_in_sat_idxs.resize(m_con_num, SIZE_MAX);
  m_con_unsat_idxs.reserve(m_con_num);
  m_con_sat_idxs.reserve(m_con_num);
  m_con_constant.resize(m_con_num, 0.0);
  m_con_activity.resize(m_con_num, 0.0);
  for (size_t con_idx = 1; con_idx < m_con_num; con_idx++)
    m_con_constant[con_idx] = m_model_manager->con(con_idx).rhs();
  if (m_explore_neighbor_list.empty())
  {
    m_explore_neighbor_list = {
        Neighbor("unsat_mtm_bm", m_bms_unsat_con, m_bms_mtm_unsat_op),
        Neighbor("sat_mtm", m_bms_sat_con, m_bms_mtm_sat_op),
        Neighbor("flip", SIZE_MAX, m_bms_flip_op),
        Neighbor("easy", SIZE_MAX, m_bms_easy_op),
        Neighbor("unsat_mtm_bm_random", SIZE_MAX, m_bms_random_op)};
  }
}

template <typename Accumulator>
bool Local_Search::solve_objective_only_impl()
{
  auto is_neg_inf_bound = [this](double bound)
  { return bound <= k_neg_inf + m_model_manager->feas_tolerance(); };
  auto is_pos_inf_bound = [this](double bound)
  { return bound >= k_inf - m_model_manager->feas_tolerance(); };
  for (size_t var_idx = 0; var_idx < m_var_num; ++var_idx)
  {
    const auto& model_var = m_model_manager->var(var_idx);
    double coeff = m_var_obj_cost[var_idx];
    double value = 0.0;
    if (is_effectively_zero(coeff, m_model_manager->zero_tolerance()))
    {
      double lower = model_var.lower_bound();
      double upper = model_var.upper_bound();
      value = 0.0;
      if (value < lower - m_model_manager->feas_tolerance())
        value = lower;
      if (value > upper + m_model_manager->feas_tolerance())
        value = upper;
      if (!std::isfinite(value))
      {
        if (std::isfinite(lower))
          value = lower;
        else if (std::isfinite(upper))
          value = upper;
        else
          value = 0.0;
      }
    }
    else if (coeff > 0)
    {
      double lower = model_var.lower_bound();
      if (is_neg_inf_bound(lower) || is_pos_inf_bound(lower))
      {
        double bound_sign = is_pos_inf_bound(lower) ? 1.0 : -1.0;
        double raw_sign = coeff * bound_sign;
        m_is_unbounded = true;
        m_is_found_feasible = false;
        m_best_obj = std::copysign(std::numeric_limits<double>::infinity(),
                                   raw_sign >= 0 ? 1.0 : -1.0);
        m_con_activity[0] = m_best_obj;
        m_min_unsat_con = 0;
        publish_best_obj();
        return true;
      }
      value = lower;
    }
    else
    {
      double upper = model_var.upper_bound();
      if (is_pos_inf_bound(upper) || is_neg_inf_bound(upper))
      {
        double bound_sign = is_pos_inf_bound(upper) ? 1.0 : -1.0;
        double raw_sign = coeff * bound_sign;
        m_is_unbounded = true;
        m_is_found_feasible = false;
        m_best_obj = std::copysign(std::numeric_limits<double>::infinity(),
                                   raw_sign >= 0 ? 1.0 : -1.0);
        m_con_activity[0] = m_best_obj;
        m_min_unsat_con = 0;
        publish_best_obj();
        return true;
      }
      value = upper;
    }
    if (!m_model_manager->normalize_var_value(model_var, value))
    {
      std::ostringstream oss;
      oss << "objective-only solution violates variable domain for '"
          << model_var.name() << "': value=" << value;
      throw Solver_Error(oss.str());
    }
    m_var_current_value[var_idx] = value;
    m_var_best_value[var_idx] = value;
  }
  const Accumulator best_obj = compute_activity<Accumulator>(
      m_model_manager->obj(), m_var_current_value.data());
  m_best_obj = static_cast<double>(best_obj);
  m_con_activity[0] = m_best_obj;
  m_is_found_feasible = true;
  m_min_unsat_con = 0;
  publish_best_obj();
  return true;
}

bool Local_Search::solve_objective_only()
{
  if (m_con_num > 1)
    return false;
  if (m_use_exact_double_activity)
    return solve_objective_only_impl<double>();
  return solve_objective_only_impl<long double>();
}

void Local_Search::normalize_domain_values(std::vector<double>& p_values,
                                           const char* p_source) const
{
  if (p_values.size() != m_var_num)
    throw Solver_Error(std::string(p_source) +
                       " size does not match variable count");
  for (size_t var_idx = 0; var_idx < m_var_num; ++var_idx)
  {
    const auto& model_var = m_model_manager->var(var_idx);
    const double original_value = p_values[var_idx];
    double normalized_value = original_value;
    if (!m_model_manager->normalize_var_value(model_var, normalized_value))
    {
      std::ostringstream oss;
      oss << p_source << " violates variable domain for '"
          << model_var.name() << "': value=" << original_value
          << ", bounds=[" << model_var.lower_bound() << ", "
          << model_var.upper_bound() << "]";
      if (model_var.requires_integrality())
        oss << ", integer variable";
      throw Solver_Error(oss.str());
    }
    p_values[var_idx] = normalized_value;
  }
}

bool Local_Search::verify_domain_values(
    const std::vector<double>& p_values) const
{
  if (p_values.size() != m_var_num)
  {
    printf("c solution size[%zu] != variable count[%zu]\n",
           p_values.size(),
           m_var_num);
    return false;
  }
  for (size_t var_idx = 0; var_idx < m_var_num; ++var_idx)
  {
    const auto& model_var = m_model_manager->var(var_idx);
    const double value = p_values[var_idx];
    if (!std::isfinite(value))
    {
      printf("c var %s has non-finite value: %.15g\n",
             model_var.name().c_str(),
             value);
      return false;
    }
    if (!m_model_manager->var_in_bound(model_var, value))
    {
      printf("c var %s is out of bound: %.15g\n",
             model_var.name().c_str(),
             value);
      return false;
    }
    if (model_var.requires_integrality() &&
        !is_integral_within_tolerance(value,
                                      m_model_manager->feas_tolerance()))
    {
      printf("c integer var %s has fractional value: %.15g\n",
             model_var.name().c_str(),
             value);
      return false;
    }
  }
  return true;
}

Local_Search::Local_Search(const Model_Manager* p_model_manager,
                           double p_opt_tolerance)
    : m_model_manager(p_model_manager),
      m_con_is_equality(p_model_manager->con_is_equality()),
      m_var_obj_cost(p_model_manager->var_obj_cost()),
      m_is_keep_feas(false), m_strct_feas(true), m_break_eq_feas(false),
      m_binary_op_stamp_token(0), m_activity_period(100000),
      m_activity_hits(0), m_activity_dirty(false),
      m_use_exact_double_activity(false), m_cur_step(0), m_tabu_base(4),
      m_tabu_variation(7), m_is_found_feasible(false),
      m_current_obj_breakthrough(false), m_last_improve_step(0),
      m_bms_unsat_con(10), m_bms_mtm_unsat_op(2250), m_bms_sat_con(1),
      m_bms_mtm_sat_op(80), m_bms_flip_op(0), m_bms_easy_op(5),
      m_bms_random_op(250), m_best_obj(k_inf),
      m_logged_obj_value(std::numeric_limits<double>::quiet_NaN()),
      m_terminated(false), m_sol_path(""), m_min_unsat_con(SIZE_MAX),
      m_has_objective(false), m_is_unbounded(false),
      m_readonly_ctx(*m_model_manager,
                     m_var_current_value,
                     m_var_best_value,
                     m_con_activity,
                     m_con_constant,
                     m_con_is_equality,
                     m_con_weight,
                     m_con_unsat_idxs,
                     m_con_pos_in_unsat_idxs,
                     m_con_sat_idxs,
                     m_var_last_dec_step,
                     m_var_last_inc_step,
                     m_var_allow_inc_step,
                     m_var_allow_dec_step,
                     m_obj_var_num,
                     m_var_obj_cost,
                     m_is_found_feasible,
                     m_best_obj,
                     m_cur_step,
                     m_last_improve_step,
                     m_current_obj_breakthrough,
                     p_model_manager->binary_idx_list(),
                     p_model_manager->non_fixed_var_idxs(),
                     p_opt_tolerance),
      m_start_ctx(m_readonly_ctx, m_var_current_value, m_rng),
      m_restart_ctx(m_readonly_ctx,
                    m_var_current_value,
                    m_rng,
                    m_con_weight),
      m_weight_ctx(m_readonly_ctx, m_rng, m_con_weight),
      m_lift_ctx(m_readonly_ctx,
                 m_rng,
                 m_best_lift_score,
                 m_best_var_idx,
                 m_best_delta,
                 m_best_age),
      m_scoring_ctx(m_readonly_ctx,
                    m_binary_op_stamp,
                    m_binary_op_stamp_token,
                    m_best_neighbor_score,
                    m_best_neighbor_subscore,
                    m_best_age,
                    m_best_var_idx,
                    m_best_delta),
      m_neighbor_ctx(m_readonly_ctx,
                     m_op_var_idxs,
                     m_op_var_deltas,
                     m_op_size,
                     m_rng),
      m_start(), m_restart(), m_weight(), m_scoring()
{
  m_rng.seed(0);
}

Local_Search::~Local_Search()
{
}
void Local_Search::terminate() noexcept
{
  m_terminated.store(true, std::memory_order_relaxed);
}

void Local_Search::set_opt_tolerance(double p_value) noexcept
{
  m_readonly_ctx.m_opt_tolerance = p_value;
}

void Local_Search::set_sol_path(const std::string& p_sol_path)
{
  m_sol_path = p_sol_path;
}

void Local_Search::set_random_seed(uint32_t p_seed)
{
  if (p_seed == 0)
  {
    m_rng.seed(0);
    return;
  }
  m_rng.seed(p_seed);
}

void Local_Search::set_start_cbk(Start_Cbk p_start_cbk, void* p_user_data)
{
  m_start.set_cbk(std::move(p_start_cbk), p_user_data);
}

void Local_Search::set_start_method(const std::string& p_method_name)
{
  m_start.set_method(p_method_name);
}

void Local_Search::set_restart_cbk(Restart::Restart_Cbk p_restart_cbk,
                                   void* p_user_data)
{
  m_restart.set_cbk(std::move(p_restart_cbk), p_user_data);
}

void Local_Search::set_restart_method(const std::string& p_restart_name)
{
  m_restart.set_method(p_restart_name);
}

void Local_Search::set_restart_step(size_t p_value)
{
  m_restart.set_restart_step(p_value);
}

void Local_Search::set_weight_cbk(Weight_Cbk p_weight_cbk,
                                  void* p_user_data)
{
  m_weight.set_cbk(std::move(p_weight_cbk), p_user_data);
}

void Local_Search::set_weight_method(const std::string& p_method_name)
{
  m_weight.set_method(p_method_name);
}

void Local_Search::set_weight_smooth_probability(
    size_t p_weight_smooth_prob)
{
  m_weight.set_smooth_probability(p_weight_smooth_prob);
}

void Local_Search::set_lift_scoring_method(
    const std::string& p_method_name)
{
  m_scoring.set_lift_method(p_method_name);
}

void Local_Search::set_neighbor_scoring_method(
    const std::string& p_method_name)
{
  m_scoring.set_neighbor_method(p_method_name);
}

void Local_Search::set_lift_scoring_cbk(Lift_Scoring_Cbk p_cbk,
                                        void* p_user_data)
{
  m_scoring.set_lift_cbk(std::move(p_cbk), p_user_data);
}

void Local_Search::set_neighbor_scoring_cbk(Neighbor_Scoring_Cbk p_cbk,
                                            void* p_user_data)
{
  m_scoring.set_neighbor_cbk(std::move(p_cbk), p_user_data);
}

void Local_Search::set_bms_unsat_con(size_t p_value)
{
  m_bms_unsat_con = p_value;
}

void Local_Search::set_bms_mtm_unsat_op(size_t p_value)
{
  m_bms_mtm_unsat_op = p_value;
}

void Local_Search::set_bms_sat_con(size_t p_value)
{
  m_bms_sat_con = p_value;
}

void Local_Search::set_bms_mtm_sat_op(size_t p_value)
{
  m_bms_mtm_sat_op = p_value;
}

void Local_Search::set_bms_flip_op(size_t p_value)
{
  m_bms_flip_op = p_value;
}

void Local_Search::set_bms_easy_op(size_t p_value)
{
  m_bms_easy_op = p_value;
}

void Local_Search::set_bms_random_op(size_t p_value)
{
  m_bms_random_op = p_value;
}

void Local_Search::clear_neighbor_list()
{
  m_explore_neighbor_list.clear();
}

void Local_Search::add_neighbor(const std::string& p_neighbor_name,
                                size_t p_bms_con,
                                size_t p_bms_op)
{
  m_explore_neighbor_list.emplace_back(
      p_neighbor_name, p_bms_con, p_bms_op);
}

void Local_Search::add_custom_neighbor(const std::string& p_neighbor_name,
                                       Neighbor_Cbk p_neighbor_cbk,
                                       void* p_user_data)
{
  m_explore_neighbor_list.emplace_back(
      p_neighbor_name, std::move(p_neighbor_cbk), p_user_data);
}

void Local_Search::reset_default_neighbor_list()
{
  m_explore_neighbor_list.clear();
  m_explore_neighbor_list = {
      Neighbor("unsat_mtm_bm", m_bms_unsat_con, m_bms_mtm_unsat_op),
      Neighbor("sat_mtm", m_bms_sat_con, m_bms_mtm_sat_op),
      Neighbor("flip", SIZE_MAX, m_bms_flip_op),
      Neighbor("easy", SIZE_MAX, m_bms_easy_op),
      Neighbor("unsat_mtm_bm_random", SIZE_MAX, m_bms_random_op)};
}

void Local_Search::set_tabu_base(size_t p_value)
{
  m_tabu_base = p_value;
}

void Local_Search::set_activity_period(size_t p_value)
{
  m_activity_period = std::max<size_t>(1, p_value);
}

void Local_Search::set_tabu_variation(size_t p_value)
{
  m_tabu_variation = std::max<size_t>(1, p_value);
}

void Local_Search::set_break_eq_feas(bool p_enable)
{
  m_break_eq_feas = p_enable;
}
