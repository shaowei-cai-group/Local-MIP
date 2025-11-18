/*=====================================================================================

    Filename:     explore_flip.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "neighbor.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <vector>

void Neighbor::explore_flip(Neighbor_Ctx& p_ctx)
{
  if (p_ctx.m_shared.m_binary_idx_list.size() == 0 || m_bms_op == 0)
    return;
  size_t neighbor_size = 0;
  auto& neighbor_idxs = sample_idxs(
      p_ctx.m_shared.m_binary_idx_list, m_bms_op, neighbor_size, p_ctx);
  for (size_t idx = 0; idx < neighbor_size; ++idx)
  {
    size_t var_idx = neighbor_idxs[idx];
    auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    assert(model_var.is_binary());
    double delta = 0;
    if (p_ctx.m_shared.m_var_current_value[var_idx] > 0.5)
      delta = -1;
    else
      delta = 1;
    if (!tabu(p_ctx, var_idx, delta))
    {
      p_ctx.m_op_var_idxs.push_back(var_idx);
      p_ctx.m_op_var_deltas.push_back(delta);
    }
  }
  p_ctx.m_op_size = p_ctx.m_op_var_deltas.size();
}
