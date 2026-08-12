/*=====================================================================================

    Filename:     neighbor_scoring.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../model_data/Model_Con.h"
#include "../../model_data/Model_Manager.h"
#include "../../model_data/Model_Var.h"
#include "../../utils/global_defs.h"
#include "../../utils/string_utils.h"
#include "../context/context.h"
#include "scoring.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void Scoring::set_neighbor_cbk(Neighbor_Cbk p_cbk, void* p_user_data)
{
  m_neighbor_cbk = std::move(p_cbk);
  m_neighbor_user_data = p_user_data;
}

void Scoring::set_neighbor_method(const std::string& p_method_name)
{
  const std::string method = string_utils::to_lower_copy(p_method_name);
  if (method.empty() || method == "progress_bonus")
    m_neighbor_method = Neighbor_Method::progress_bonus;
  else if (method == "progress_age")
    m_neighbor_method = Neighbor_Method::progress_age;
  else
    throw std::invalid_argument("unsupported neighbor scoring method: " +
                                p_method_name);
}

Scoring::Neighbor_Ctx::Neighbor_Ctx(
    const Readonly_Ctx& p_shared,
    std::vector<uint32_t>& p_binary_op_stamp,
    uint32_t& p_binary_op_stamp_token,
    long& p_current_neighbor_score,
    long& p_current_neighbor_subscore,
    size_t& p_current_best_age,
    size_t& p_current_best_var_idx,
    double& p_current_best_delta)
    : m_shared(p_shared), m_binary_op_stamp(p_binary_op_stamp),
      m_binary_op_stamp_token(p_binary_op_stamp_token),
      m_best_neighbor_score(p_current_neighbor_score),
      m_best_neighbor_subscore(p_current_neighbor_subscore),
      m_best_age(p_current_best_age),
      m_best_var_idx(p_current_best_var_idx),
      m_best_delta(p_current_best_delta)
{
}

void Scoring::score_neighbor(Neighbor_Ctx& p_ctx,
                             size_t p_var_idx,
                             double p_delta) const
{
  if (m_neighbor_cbk)
  {
    m_neighbor_cbk(p_ctx, p_var_idx, p_delta, m_neighbor_user_data);
    return;
  }
  progress(p_ctx,
           p_var_idx,
           p_delta,
           m_neighbor_method == Neighbor_Method::progress_bonus);
}

void Scoring::progress(Neighbor_Ctx& p_ctx,
                       size_t p_var_idx,
                       double p_delta,
                       bool p_use_bonus) const
{
  const auto& model_manager = p_ctx.m_shared.m_model_manager;
  const auto& model_var = model_manager.var(p_var_idx);
  const double feas_tolerance = model_manager.feas_tolerance();
  if (model_var.type() == Var_Type::binary)
  {
    if (p_ctx.m_binary_op_stamp[p_var_idx] ==
        p_ctx.m_binary_op_stamp_token)
      return;
    p_ctx.m_binary_op_stamp[p_var_idx] = p_ctx.m_binary_op_stamp_token;
  }
  long neighbor_score = 0;
  long bonus_score = 0;
  const size_t term_num = model_var.term_num();
  if (term_num == 0)
    return;
  for (size_t term_idx = 0; term_idx < term_num; ++term_idx)
  {
    const size_t con_idx = model_var.con_idx(term_idx);
    const size_t pos_in_con = model_var.pos_in_con(term_idx);
    const auto& model_con = model_manager.con(con_idx);
    const long con_weight =
        static_cast<long>(p_ctx.m_shared.m_con_weight[con_idx]);
    const long scaled_con_weight = con_weight * 2;
    if (con_idx == 0 && p_ctx.m_shared.m_is_found_feasible)
    {
      double new_obj = p_ctx.m_shared.m_con_activity[con_idx] +
                       model_con.coeff(pos_in_con) * p_delta;
      if (new_obj < p_ctx.m_shared.m_con_activity[con_idx])
        neighbor_score += scaled_con_weight;
      else
        neighbor_score -= scaled_con_weight;
      if (p_use_bonus && new_obj < p_ctx.m_shared.m_best_obj)
        bonus_score += con_weight;
    }
    else
    {
      double new_activity = p_ctx.m_shared.m_con_activity[con_idx] +
                            model_con.coeff(pos_in_con) * p_delta;
      double pre_gap = p_ctx.m_shared.m_con_activity[con_idx] -
                       p_ctx.m_shared.m_con_constant[con_idx];
      double new_gap =
          new_activity - p_ctx.m_shared.m_con_constant[con_idx];
      bool pre_sat;
      if (p_ctx.m_shared.m_con_is_equality[con_idx])
      {
        pre_sat = std::fabs(pre_gap) <= feas_tolerance;
        bool now_sat = std::fabs(new_gap) <= feas_tolerance;
        if (!pre_sat && now_sat)
          neighbor_score += scaled_con_weight * 2;
        else if (pre_sat && !now_sat)
          neighbor_score -= scaled_con_weight * 2;
        else if (!pre_sat && !now_sat)
        {
          if (std::fabs(new_gap) < std::fabs(pre_gap))
            neighbor_score += scaled_con_weight;
          else
            neighbor_score -= scaled_con_weight;
        }
      }
      else
      {
        pre_sat = pre_gap <= feas_tolerance;
        bool now_sat = new_gap <= feas_tolerance;
        if (!pre_sat && now_sat)
          neighbor_score += scaled_con_weight;
        else if (pre_sat && !now_sat)
          neighbor_score -= scaled_con_weight;
        else if (!pre_sat && !now_sat)
        {
          if (new_gap < pre_gap)
            neighbor_score += con_weight;
          else
            neighbor_score -= con_weight;
        }
      }
    }
  }
  const size_t age =
      std::max(p_ctx.m_shared.m_var_last_dec_step[p_var_idx],
               p_ctx.m_shared.m_var_last_inc_step[p_var_idx]);
  bool is_better = p_ctx.m_best_neighbor_score < neighbor_score;
  if (p_ctx.m_best_neighbor_score == neighbor_score)
  {
    is_better = p_use_bonus
                    ? p_ctx.m_best_neighbor_subscore < bonus_score ||
                          (p_ctx.m_best_neighbor_subscore == bonus_score &&
                           age < p_ctx.m_best_age)
                    : age < p_ctx.m_best_age;
  }
  if (!is_better)
    return;

  p_ctx.m_best_var_idx = p_var_idx;
  p_ctx.m_best_delta = p_delta;
  p_ctx.m_best_neighbor_score = neighbor_score;
  if (p_use_bonus)
    p_ctx.m_best_neighbor_subscore = bonus_score;
  p_ctx.m_best_age = age;
}
