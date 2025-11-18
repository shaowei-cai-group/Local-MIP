/*=====================================================================================

    Filename:     weight.cpp

    Description:  Weight update strategies for constraint violation management
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../model_data/Model_Manager.h"
#include "../context/context.h"
#include "weight.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

Weight::Weight_Ctx::Weight_Ctx(const Readonly_Ctx& p_shared,
                               std::mt19937& p_rng,
                               std::vector<size_t>& p_con_weight)
    : m_shared(p_shared), m_rng(p_rng), m_con_weight(p_con_weight)
{
}

Weight::Weight()
    : m_user_cbk(nullptr), m_user_data(nullptr),
      m_default_method(Method::monotone), m_smooth_prob(1)
{
}

void Weight::set_cbk(Weight_Cbk p_weight_cbk, void* p_user_data)
{
  m_user_cbk = std::move(p_weight_cbk);
  m_user_data = p_user_data;
}

void Weight::set_method(const std::string& p_method_name)
{
  std::string method = p_method_name;
  std::transform(method.begin(),
                 method.end(),
                 method.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (method.empty() || method == "smooth")
    m_default_method = Method::smooth;
  else if (method == "monotone")
    m_default_method = Method::monotone;
  else
  {
    printf("c unsupported weight method %s, fallback to smooth.\n",
           p_method_name.c_str());
    m_default_method = Method::smooth;
  }
}

void Weight::set_smooth_probability(size_t p_weight_smooth_prob)
{
  m_smooth_prob = p_weight_smooth_prob;
}

const size_t& Weight::smooth_probability() const
{
  return m_smooth_prob;
}

void Weight::update(Weight_Ctx& p_ctx) const
{
  if (m_user_cbk)
  {
    m_user_cbk(p_ctx, m_user_data);
    return;
  }
  if (m_default_method == Method::monotone)
    monotone_update(p_ctx);
  else
    smooth_update(p_ctx);
}

void Weight::smooth_update(Weight_Ctx& p_ctx) const
{
  if (p_ctx.m_rng() % 10000 > m_smooth_prob)
  {
    for (size_t con_idx : p_ctx.m_shared.m_con_unsat_idxs)
      p_ctx.m_con_weight[con_idx]++;
    if (p_ctx.m_shared.m_is_found_feasible &&
        p_ctx.m_shared.m_con_unsat_idxs.empty())
      p_ctx.m_con_weight[0]++;
  }
  else
  {
    size_t con_num = p_ctx.m_shared.m_model_manager.con_num();
    for (size_t con_idx = 1; con_idx < con_num; ++con_idx)
    {
      bool is_sat =
          p_ctx.m_shared.m_con_pos_in_unsat_idxs[con_idx] == SIZE_MAX;
      if (is_sat && p_ctx.m_con_weight[con_idx] > 0)
        p_ctx.m_con_weight[con_idx]--;
    }
    if (p_ctx.m_shared.m_is_found_feasible &&
        p_ctx.m_shared.m_current_obj_breakthrough &&
        p_ctx.m_con_weight[0] > 0)
      p_ctx.m_con_weight[0]--;
  }
}

void Weight::monotone_update(Weight_Ctx& p_ctx) const
{
  for (size_t con_idx : p_ctx.m_shared.m_con_unsat_idxs)
    p_ctx.m_con_weight[con_idx]++;
  if (p_ctx.m_shared.m_is_found_feasible &&
      p_ctx.m_shared.m_con_unsat_idxs.empty())
    p_ctx.m_con_weight[0]++;
}
