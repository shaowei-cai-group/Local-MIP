/*=====================================================================================

    Filename:     neighbor.cpp

    Description:  Search neighbor mechanisms and strategies
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../utils/global_defs.h"
#include "../context/context.h"
#include "neighbor.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::vector<size_t> Neighbor::m_bms_idxs;
std::unordered_map<size_t, size_t> Neighbor::m_remap;

Neighbor::Neighbor_Ctx::Neighbor_Ctx(const Readonly_Ctx& p_shared,
                                     std::vector<size_t>& p_op_var_idxs,
                                     std::vector<double>& p_op_var_deltas,
                                     size_t& p_op_size,
                                     std::mt19937& p_rng)
    : m_shared(p_shared), m_op_var_idxs(p_op_var_idxs),
      m_op_var_deltas(p_op_var_deltas), m_op_size(p_op_size), m_rng(p_rng)
{
}

Neighbor::Neighbor(const std::string& p_neighbor_name,
                   size_t p_bms_con,
                   size_t p_bms_op)
    : m_neighbor_cbk(nullptr), m_user_data(nullptr), m_bms_con(p_bms_con),
      m_bms_op(p_bms_op)
{
  std::string method = p_neighbor_name;
  std::transform(method.begin(),
                 method.end(),
                 method.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (method.empty() || method == "unsat_mtm_bm")
    m_strategy = Strategy::unsat_mtm_bm;
  else if (method == "sat_mtm")
    m_strategy = Strategy::sat_mtm;
  else if (method == "flip")
    m_strategy = Strategy::flip;
  else if (method == "easy")
    m_strategy = Strategy::easy;
  else if (method == "unsat_mtm_bm_random")
    m_strategy = Strategy::unsat_mtm_bm_random;
  else
  {
    printf("c unsupported neighbor method %s, fallback to unsat_mtm_bm.\n",
           p_neighbor_name.c_str());
    m_strategy = Strategy::unsat_mtm_bm;
  }
}

Neighbor::Neighbor(const std::string& p_neighbor_name,
                   Neighbor_Cbk p_neighbor_cbk,
                   void* p_user_data)
    : m_strategy(Strategy::user_defined),
      m_neighbor_cbk(std::move(p_neighbor_cbk)), m_user_data(p_user_data),
      m_bms_con(0), m_bms_op(0)
{
  (void)p_neighbor_name;
}

void Neighbor::set_cbk(Neighbor_Cbk p_neighbor_cbk, void* p_user_data)
{
  m_strategy = Strategy::user_defined;
  m_neighbor_cbk = std::move(p_neighbor_cbk);
  m_user_data = p_user_data;
}

void Neighbor::explore(Neighbor_Ctx& p_ctx)
{
  switch (m_strategy)
  {
    case Strategy::unsat_mtm_bm:
      explore_unsat_mtm_bm(p_ctx);
      break;
    case Strategy::sat_mtm:
      explore_sat_mtm(p_ctx);
      break;
    case Strategy::flip:
      explore_flip(p_ctx);
      break;
    case Strategy::easy:
      explore_easy(p_ctx);
      break;
    case Strategy::unsat_mtm_bm_random:
      explore_unsat_random_bm(p_ctx);
      break;
    case Strategy::user_defined:
      m_neighbor_cbk(p_ctx, m_user_data);
      break;
  }
}

double Neighbor::breakthrough_operation(size_t p_term_idx,
                                        size_t p_var_idx,
                                        Neighbor_Ctx& p_ctx)
{
  assert(!p_ctx.m_shared.m_current_obj_breakthrough);
  auto& model_obj = p_ctx.m_shared.m_model_manager.obj();
  auto& model_var = p_ctx.m_shared.m_model_manager.var(p_var_idx);
  double gap =
      p_ctx.m_shared.m_con_activity[0] - p_ctx.m_shared.m_con_constant[0];
  double coeff = model_obj.coeff(p_term_idx);
  if (std::fabs(coeff) < k_zero_tolerance)
    return 0;
  double delta = -(gap / coeff);
  if (!model_var.is_real())
  {
    if (coeff > 0)
      delta = std::floor(delta);
    else
      delta = std::ceil(delta);
  }
  if (!model_var.in_bound(p_ctx.m_shared.m_var_current_value[p_var_idx] +
                          delta))
  {
    if (coeff > 0)
      delta = model_var.lower_bound() -
              p_ctx.m_shared.m_var_current_value[p_var_idx];
    else
      delta = model_var.upper_bound() -
              p_ctx.m_shared.m_var_current_value[p_var_idx];
  }
  return delta;
}

double Neighbor::inequality_mixed_tight_operation(size_t p_con_idx,
                                                  size_t p_term_idx,
                                                  size_t p_var_idx,
                                                  Neighbor_Ctx& p_ctx)
{
  auto& model_con = p_ctx.m_shared.m_model_manager.con(p_con_idx);
  auto& model_var = p_ctx.m_shared.m_model_manager.var(p_var_idx);
  double gap = p_ctx.m_shared.m_con_activity[p_con_idx] -
               p_ctx.m_shared.m_con_constant[p_con_idx];
  double coeff = model_con.coeff(p_term_idx);
  if (std::fabs(coeff) < k_zero_tolerance)
    return 0;
  double delta = -(gap / coeff);
  if (!model_var.is_real())
  {
    if (coeff > 0)
      delta = std::floor(delta);
    else
      delta = std::ceil(delta);
  }
  if (!model_var.in_bound(p_ctx.m_shared.m_var_current_value[p_var_idx] +
                          delta))
  {
    if (p_ctx.m_shared.m_con_pos_in_unsat_idxs[p_con_idx] != SIZE_MAX)
    {
      if (coeff > 0)
        delta = model_var.lower_bound() -
                p_ctx.m_shared.m_var_current_value[p_var_idx];
      else
        delta = model_var.upper_bound() -
                p_ctx.m_shared.m_var_current_value[p_var_idx];
    }
    else
    {
      if (coeff > 0)
        delta = model_var.upper_bound() -
                p_ctx.m_shared.m_var_current_value[p_var_idx];
      else
        delta = model_var.lower_bound() -
                p_ctx.m_shared.m_var_current_value[p_var_idx];
    }
  }
  return delta;
}

double Neighbor::equality_mixed_tight_operation(size_t p_con_idx,
                                                size_t p_term_idx,
                                                size_t p_var_idx,
                                                Neighbor_Ctx& p_ctx)
{
  auto& model_con = p_ctx.m_shared.m_model_manager.con(p_con_idx);
  auto& model_var = p_ctx.m_shared.m_model_manager.var(p_var_idx);
  double gap = p_ctx.m_shared.m_con_activity[p_con_idx] -
               p_ctx.m_shared.m_con_constant[p_con_idx];
  double coeff = model_con.coeff(p_term_idx);
  if (std::fabs(coeff) < k_zero_tolerance)
    return 0;
  double delta = -(gap / coeff);
  if (!model_var.is_real())
    delta = std::round(delta);
  if (!model_var.in_bound(p_ctx.m_shared.m_var_current_value[p_var_idx] +
                          delta))
  {
    if ((gap > 0 && coeff > 0) || (gap < 0 && coeff < 0))
      delta = model_var.lower_bound() -
              p_ctx.m_shared.m_var_current_value[p_var_idx];
    else
      delta = model_var.upper_bound() -
              p_ctx.m_shared.m_var_current_value[p_var_idx];
  }
  return delta;
}