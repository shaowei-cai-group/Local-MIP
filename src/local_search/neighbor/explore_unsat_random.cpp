/*=====================================================================================

    Filename:     explore_unsat_random.cpp

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
#include <random>
#include <vector>


void Neighbor::explore_unsat_random_bm(Neighbor_Ctx& p_ctx)
{
  if (p_ctx.m_shared.m_con_unsat_idxs.size() > 0)
  {
    std::uniform_int_distribution<size_t> dist(
        0, p_ctx.m_shared.m_con_unsat_idxs.size() - 1);
    size_t con_idx = p_ctx.m_shared.m_con_unsat_idxs[dist(p_ctx.m_rng)];
    auto& model_con = p_ctx.m_shared.m_model_manager.con(con_idx);
    for (size_t term_idx = 0; term_idx < model_con.term_num(); ++term_idx)
    {
      size_t var_idx = model_con.var_idx(term_idx);
      double delta;
      if (p_ctx.m_shared.m_con_is_equality[con_idx])
        delta = equality_mixed_tight_operation(
            con_idx, term_idx, var_idx, p_ctx);
      else
        delta = inequality_mixed_tight_operation(
            con_idx, term_idx, var_idx, p_ctx);
      if (tabu_latest(p_ctx, var_idx, delta))
        continue;
      if (std::fabs(delta) < k_zero_tolerance)
        continue;
      p_ctx.m_op_var_idxs.push_back(var_idx);
      p_ctx.m_op_var_deltas.push_back(delta);
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
      if (tabu_latest(p_ctx, var_idx, delta))
        continue;
      if (std::fabs(delta) < k_zero_tolerance)
        continue;
      p_ctx.m_op_var_idxs.push_back(var_idx);
      p_ctx.m_op_var_deltas.push_back(delta);
    }
  p_ctx.m_op_size = sample_op(
      m_bms_op, p_ctx.m_op_var_idxs, p_ctx.m_op_var_deltas, p_ctx);
}
