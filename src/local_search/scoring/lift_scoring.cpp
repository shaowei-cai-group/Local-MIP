/*=====================================================================================

    Filename:     lift_scoring.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../utils/global_defs.h"
#include "../context/context.h"
#include "scoring.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

void Scoring::set_lift_cbk(Lift_Cbk p_cbk, void* p_user_data)
{
  m_lift_cbk = std::move(p_cbk);
  m_lift_user_data = p_user_data;
}

Scoring::Lift_Ctx::Lift_Ctx(const Readonly_Ctx& p_shared,
                            std::mt19937& p_rng,
                            double& p_best_lift_score,
                            size_t& p_current_best_var_idx,
                            double& p_current_best_delta,
                            size_t& p_current_best_age)
    : m_shared(p_shared), m_rng(p_rng),
      m_best_lift_score(p_best_lift_score),
      m_best_var_idx(p_current_best_var_idx),
      m_best_delta(p_current_best_delta), m_best_age(p_current_best_age)
{
}

void Scoring::score_lift(Lift_Ctx& p_ctx,
                         size_t p_var_idx,
                         double p_delta) const
{
  if (m_lift_cbk)
  {
    m_lift_cbk(p_ctx, p_var_idx, p_delta, m_lift_user_data);
    return;
  }
  if (m_lift_method == Lift_Method::lift_random)
    lift_random(p_ctx, p_var_idx, p_delta);
  else
    lift_age(p_ctx, p_var_idx, p_delta);
}

void Scoring::lift_age(Lift_Ctx& p_ctx,
                       size_t p_var_idx,
                       double p_delta) const
{
  double lift_score = -p_ctx.m_shared.m_var_obj_cost[p_var_idx] * p_delta;
  size_t age = std::max(p_ctx.m_shared.m_var_last_dec_step[p_var_idx],
                        p_ctx.m_shared.m_var_last_inc_step[p_var_idx]);
  if (p_ctx.m_best_lift_score + k_opt_tolerance < lift_score ||
      (p_ctx.m_best_lift_score <= lift_score && age < p_ctx.m_best_age))
  {
    p_ctx.m_best_var_idx = p_var_idx;
    p_ctx.m_best_delta = p_delta;
    p_ctx.m_best_lift_score = lift_score;
    p_ctx.m_best_age = age;
  }
}

void Scoring::lift_random(Lift_Ctx& p_ctx,
                          size_t p_var_idx,
                          double p_delta) const
{
  double lift_score = -p_ctx.m_shared.m_var_obj_cost[p_var_idx] * p_delta;
  size_t age = std::max(p_ctx.m_shared.m_var_last_dec_step[p_var_idx],
                        p_ctx.m_shared.m_var_last_inc_step[p_var_idx]);
  if (p_ctx.m_best_var_idx == SIZE_MAX ||
      p_ctx.m_best_lift_score + k_opt_tolerance < lift_score)
  {
    p_ctx.m_best_var_idx = p_var_idx;
    p_ctx.m_best_delta = p_delta;
    p_ctx.m_best_lift_score = lift_score;
    p_ctx.m_best_age = age;
    return;
  }
  if (p_ctx.m_best_lift_score <= lift_score)
  {
    if ((p_ctx.m_rng() & 1U) != 0)
    {
      p_ctx.m_best_var_idx = p_var_idx;
      p_ctx.m_best_delta = p_delta;
      p_ctx.m_best_lift_score = lift_score;
      p_ctx.m_best_age = age;
    }
  }
}

void Scoring::set_lift_method(const std::string& p_method_name)
{
  std::string method = p_method_name;
  std::transform(method.begin(),
                 method.end(),
                 method.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (method.empty() || method == "lift_age")
    m_lift_method = Lift_Method::lift_age;
  else if (method == "lift_random")
    m_lift_method = Lift_Method::lift_random;
  else
  {
    printf("c unsupported lift scoring method %s, fallback to "
           "lift_age.\n",
           p_method_name.c_str());
    m_lift_method = Lift_Method::lift_age;
  }
}
