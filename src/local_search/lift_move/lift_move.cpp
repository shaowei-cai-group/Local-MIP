/*=====================================================================================

    Filename:     lift_move.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../utils/global_defs.h"
#include "../Local_Search.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>


bool Local_Search::lift_move()
{
  reset_op(true);
  m_strct_feas = true;
  auto& model_obj = m_model_manager->obj();
  if (!m_is_keep_feas)
  {
    for (size_t term_idx = 0; term_idx < m_obj_var_num; ++term_idx)
    {
      size_t var_idx = model_obj.var_idx(term_idx);
      double delta = lift_move_operation(term_idx, var_idx);
      m_var_lift_delta[term_idx] = delta;
    }
  }
  for (size_t term_idx = 0; term_idx < m_obj_var_num; ++term_idx)
    m_scoring.score_lift(m_lift_ctx,
                         model_obj.var_idx(term_idx),
                         m_var_lift_delta[term_idx]);
  if (m_best_var_idx != SIZE_MAX && m_best_delta != 0)
  {
    size_t obj_term_idx =
        m_model_manager->var_id_to_obj_idx(m_best_var_idx);
    apply_move(m_best_var_idx, m_best_delta);
    if (obj_term_idx != SIZE_MAX)
      m_var_lift_delta[obj_term_idx] =
          lift_move_operation(obj_term_idx, m_best_var_idx);
    m_is_keep_feas = true;
    m_feas_touch_vars.clear();
    auto& best_model_var = m_model_manager->var(m_best_var_idx);
    for (auto con_idx : best_model_var.con_idx_set())
    {
      if (con_idx == 0)
        continue;
      auto& model_con = m_model_manager->con(con_idx);
      for (auto var_idx : model_con.var_idx_set())
        m_feas_touch_vars.insert(var_idx);
    }
    for (auto var_idx : m_feas_touch_vars)
    {
      size_t obj_term_idx = m_model_manager->var_id_to_obj_idx(var_idx);
      if (obj_term_idx == SIZE_MAX)
        continue;
      m_var_lift_delta[obj_term_idx] =
          lift_move_operation(obj_term_idx, var_idx);
    }
    return true;
  }
  if (m_break_eq_feas)
  {
    m_is_keep_feas = false;
    m_strct_feas = false;
    auto& model_obj = m_model_manager->obj();
    for (size_t term_idx = 0; term_idx < m_obj_var_num; ++term_idx)
    {
      size_t var_idx = model_obj.var_idx(term_idx);
      double delta = lift_move_operation(term_idx, var_idx);
      m_var_lift_delta[term_idx] = delta;
    }
    for (size_t term_idx = 0; term_idx < m_obj_var_num; ++term_idx)
      m_scoring.score_lift(m_lift_ctx,
                           model_obj.var_idx(term_idx),
                           m_var_lift_delta[term_idx]);
    if (m_best_var_idx != SIZE_MAX && m_best_delta != 0)
      apply_move(m_best_var_idx, m_best_delta);
    return false;
  }
  m_is_keep_feas = false;
  return false;
}

double Local_Search::lift_move_operation(size_t p_term_idx,
                                         size_t p_var_idx)
{
  auto& model_var = m_model_manager->var(p_var_idx);
  m_var_LB_feas_delta[p_term_idx] =
      model_var.lower_bound() - m_var_current_value[p_var_idx];
  m_var_UB_feas_delta[p_term_idx] =
      model_var.upper_bound() - m_var_current_value[p_var_idx];
  for (size_t var_term_idx = 0; var_term_idx < model_var.term_num();
       ++var_term_idx)
  {
    size_t con_idx = model_var.con_idx(var_term_idx);
    auto& model_con = m_model_manager->con(con_idx);
    size_t pos_in_con = model_var.pos_in_con(var_term_idx);
    double coeff = model_con.coeff(pos_in_con);
    if (std::fabs(coeff) < k_zero_tolerance)
      continue;
    if (con_idx == 0)
      continue;
    long double ld_gap =
        static_cast<long double>(m_con_activity[con_idx]) -
        static_cast<long double>(m_con_constant[con_idx]);
    double gap = static_cast<double>(ld_gap);
    long double ld_delta = -(ld_gap / static_cast<long double>(coeff));
    double delta = static_cast<double>(ld_delta);
    if (m_con_is_equality[con_idx])
    {
      if (m_strct_feas)
      {
        m_var_LB_feas_delta[p_term_idx] = 0;
        m_var_UB_feas_delta[p_term_idx] = 0;
      }
    }
    else
    {
      if (gap >= 0)
      {
        if (coeff > 0)
          m_var_UB_feas_delta[p_term_idx] = 0;
        else
          m_var_LB_feas_delta[p_term_idx] = 0;
      }
      else
      {
        if (coeff > 0)
        {
          if (!model_var.is_real())
            delta = std::floor(delta);
          if (delta < m_var_UB_feas_delta[p_term_idx])
            m_var_UB_feas_delta[p_term_idx] = delta;
        }
        else if (coeff < 0)
        {
          if (!model_var.is_real())
            delta = std::ceil(delta);
          if (delta > m_var_LB_feas_delta[p_term_idx])
            m_var_LB_feas_delta[p_term_idx] = delta;
        }
      }
    }
    if (m_var_LB_feas_delta[p_term_idx] >= m_var_UB_feas_delta[p_term_idx])
      break;
  }
  double candidate_delta = (m_var_obj_cost[p_var_idx] > 0)
                               ? m_var_LB_feas_delta[p_term_idx]
                               : m_var_UB_feas_delta[p_term_idx];
  return candidate_delta;
}
