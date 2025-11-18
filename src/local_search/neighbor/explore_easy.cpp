/*=====================================================================================

    Filename:     explore_easy.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../utils/global_defs.h"
#include "neighbor.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>

void Neighbor::explore_easy(Neighbor_Ctx& p_ctx)
{
  if (m_bms_op == 0)
    return;
  size_t neighbor_size = 0;
  auto& neighbor_idxs =
      sample_idxs(p_ctx.m_shared.m_non_fixed_var_idx_list,
                  m_bms_op,
                  neighbor_size,
                  p_ctx);
  for (size_t idx = 0; idx < neighbor_size; ++idx)
  {
    size_t var_idx = neighbor_idxs[idx];
    auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    double delta = 0;
    if (model_var.lower_bound() > 0)
      delta = model_var.lower_bound() -
              p_ctx.m_shared.m_var_current_value[var_idx];
    else if (model_var.upper_bound() < 0)
      delta = model_var.upper_bound() -
              p_ctx.m_shared.m_var_current_value[var_idx];
    else
      delta = 0 - p_ctx.m_shared.m_var_current_value[var_idx];
    if (std::fabs(delta) > k_feas_tolerance &&
        !tabu(p_ctx, var_idx, delta))
    {
      p_ctx.m_op_var_idxs.push_back(var_idx);
      p_ctx.m_op_var_deltas.push_back(delta);
    }
    bool has_finite_lower = model_var.lower_bound() > k_neg_inf * 0.5;
    bool has_finite_upper = model_var.upper_bound() < k_inf * 0.5;
    if (model_var.is_real() && has_finite_lower && has_finite_upper)
    {
      delta =
          std::midpoint(model_var.lower_bound(), model_var.upper_bound()) -
          p_ctx.m_shared.m_var_current_value[var_idx];
      if (std::fabs(delta) > k_feas_tolerance &&
          !tabu(p_ctx, var_idx, delta))
      {
        p_ctx.m_op_var_idxs.push_back(var_idx);
        p_ctx.m_op_var_deltas.push_back(delta);
      }
    }
    if (has_finite_lower && model_var.lower_bound() < 0)
    {
      delta = model_var.lower_bound() -
              p_ctx.m_shared.m_var_current_value[var_idx];
      if (std::fabs(delta) > k_feas_tolerance &&
          !tabu(p_ctx, var_idx, delta))
      {
        p_ctx.m_op_var_idxs.push_back(var_idx);
        p_ctx.m_op_var_deltas.push_back(delta);
      }
    }
    if (has_finite_upper && model_var.upper_bound() > 0)
    {
      delta = model_var.upper_bound() -
              p_ctx.m_shared.m_var_current_value[var_idx];
      if (std::fabs(delta) > k_feas_tolerance &&
          !tabu(p_ctx, var_idx, delta))
      {
        p_ctx.m_op_var_idxs.push_back(var_idx);
        p_ctx.m_op_var_deltas.push_back(delta);
      }
    }
  }
  p_ctx.m_op_size = p_ctx.m_op_var_deltas.size();
}
