/*=====================================================================================

    Filename:     explore_sat.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../utils/global_defs.h"
#include "neighbor.h"
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

void Neighbor::explore_sat_mtm(Neighbor_Ctx& p_ctx)
{
  if (p_ctx.m_shared.m_model_manager.con_num() <= 1 ||
      !p_ctx.m_shared.m_is_found_feasible || m_bms_con == 0 ||
      m_bms_op == 0)
    return;
  if (p_ctx.m_shared.m_con_sat_idxs.size() > 0)
  {
    size_t neighbor_size = 0;
    auto& neighbor_con_idxs = sample_idxs(
        p_ctx.m_shared.m_con_sat_idxs, m_bms_con, neighbor_size, p_ctx);
    for (size_t neighbor_idx = 0; neighbor_idx < neighbor_size;
         ++neighbor_idx)
    {
      size_t con_idx = neighbor_con_idxs.at(neighbor_idx);
      auto& model_con = p_ctx.m_shared.m_model_manager.con(con_idx);
      if (p_ctx.m_shared.m_con_is_equality[con_idx] ||
          model_con.is_inferred_sat())
        continue;
      for (size_t term_idx = 0; term_idx < model_con.term_num();
           ++term_idx)
      {
        size_t var_idx = model_con.var_idx(term_idx);
        double delta = inequality_mixed_tight_operation(
            con_idx, term_idx, var_idx, p_ctx);
        if (tabu(p_ctx, var_idx, delta))
          continue;
        if (std::fabs(delta) < k_zero_tolerance)
          continue;
        p_ctx.m_op_var_idxs.push_back(var_idx);
        p_ctx.m_op_var_deltas.push_back(delta);
      }
    }
  }
  p_ctx.m_op_size = sample_op(
      m_bms_op, p_ctx.m_op_var_idxs, p_ctx.m_op_var_deltas, p_ctx);
}
