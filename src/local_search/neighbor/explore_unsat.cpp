/*=====================================================================================

    Filename:     explore_unsat.cpp

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

void Neighbor::explore_unsat_mtm_bm(Neighbor_Ctx& p_ctx)
{
  if (m_bms_con == 0 || m_bms_op == 0)
    return;
  if (p_ctx.m_shared.m_con_unsat_idxs.size() > 0)
  {
    size_t neighbor_size = 0;
    auto& neighbor_con_idxs = sample_idxs(
        p_ctx.m_shared.m_con_unsat_idxs, m_bms_con, neighbor_size, p_ctx);
    for (size_t neighbor_idx = 0; neighbor_idx < neighbor_size;
         ++neighbor_idx)
    {
      size_t con_idx = neighbor_con_idxs.at(neighbor_idx);
      auto& model_con = p_ctx.m_shared.m_model_manager.con(con_idx);
      for (size_t term_idx = 0; term_idx < model_con.term_num();
           ++term_idx)
      {
        size_t var_idx = model_con.var_idx(term_idx);
        double delta;
        if (p_ctx.m_shared.m_con_is_equality[con_idx])
          delta = equality_mixed_tight_operation(
              con_idx, term_idx, var_idx, p_ctx);
        else
          delta = inequality_mixed_tight_operation(
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
  auto& model_obj = p_ctx.m_shared.m_model_manager.obj();
  if (p_ctx.m_shared.m_is_found_feasible &&
      !p_ctx.m_shared.m_current_obj_breakthrough)
    for (size_t term_idx = 0; term_idx < p_ctx.m_shared.m_obj_var_num;
         ++term_idx)
    {
      size_t var_idx = model_obj.var_idx(term_idx);
      double delta = breakthrough_operation(term_idx, var_idx, p_ctx);
      if (tabu(p_ctx, var_idx, delta))
        continue;
      if (std::fabs(delta) < k_zero_tolerance)
        continue;
      p_ctx.m_op_var_idxs.push_back(var_idx);
      p_ctx.m_op_var_deltas.push_back(delta);
    }
  p_ctx.m_op_size = sample_op(
      m_bms_op, p_ctx.m_op_var_idxs, p_ctx.m_op_var_deltas, p_ctx);
}
