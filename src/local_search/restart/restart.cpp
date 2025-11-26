/*=====================================================================================

    Filename:     restart.cpp

    Description:  Search restart mechanisms and strategies
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../model_data/Model_Manager.h"
#include "../../model_data/Model_Var.h"
#include "../../utils/global_defs.h"
#include "../context/context.h"
#include "restart.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

Restart::Restart_Ctx::Restart_Ctx(const Readonly_Ctx& p_shared,
                                  std::vector<double>& p_current_values,
                                  std::mt19937& p_rng,
                                  std::vector<size_t>& p_con_weight)
    : m_shared(p_shared), m_var_current_value(p_current_values),
      m_rng(p_rng), m_con_weight(p_con_weight)
{
}

Restart::Restart()
    : m_user_cbk(nullptr), m_user_data(nullptr),
      m_default_strategy(Strategy::best), m_restart_step(1000000)
{
}

void Restart::set_cbk(Restart_Cbk p_restart_cbk, void* p_user_data)
{
  m_user_cbk = std::move(p_restart_cbk);
  m_user_data = p_user_data;
}

void Restart::set_method(const std::string& p_restart_name)
{
  std::string method = p_restart_name;
  std::transform(method.begin(),
                 method.end(),
                 method.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (method.empty() || method == "random")
    m_default_strategy = Strategy::random;
  else if (method == "best")
    m_default_strategy = Strategy::best;
  else if (method == "hybrid")
    m_default_strategy = Strategy::hybrid;
  else
  {
    printf("c unsupported restart method %s, fallback to random.\n",
           p_restart_name.c_str());
    m_default_strategy = Strategy::random;
  }
}

void Restart::set_restart_step(size_t p_restart_step)
{
  m_restart_step = p_restart_step;
}

bool Restart::execute(Restart_Ctx& p_ctx) const
{
  if (!should_restart(p_ctx))
    return false;
  if (m_user_cbk)
  {
    m_user_cbk(p_ctx, m_user_data);
    return true;
  }
  switch (m_default_strategy)
  {
    case Strategy::best:
      best_restart(p_ctx);
      break;
    case Strategy::hybrid:
      hybrid_restart(p_ctx);
      break;
    case Strategy::random:
    default:
      random_restart(p_ctx);
      break;
  }
  return true;
}

void Restart::reset_weights(Restart_Ctx& p_ctx) const
{
  std::fill(p_ctx.m_con_weight.begin(), p_ctx.m_con_weight.end(), 1);
}

void Restart::random_restart(Restart_Ctx& p_ctx) const
{
  size_t var_num = p_ctx.m_shared.m_model_manager.var_num();
  for (size_t var_idx = 0; var_idx < var_num; ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    double value = sample_random_value(p_ctx, model_var);
    p_ctx.m_var_current_value[var_idx] = value;
    assert(model_var.in_bound(value));
  }
  reset_weights(p_ctx);
}

void Restart::best_restart(Restart_Ctx& p_ctx) const
{
  if (!p_ctx.m_shared.m_is_found_feasible)
  {
    random_restart(p_ctx);
    return;
  }
  size_t var_num = p_ctx.m_shared.m_model_manager.var_num();
  for (size_t var_idx = 0; var_idx < var_num; ++var_idx)
  {
    double value = p_ctx.m_shared.m_var_best_value[var_idx];
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    double lower = model_var.lower_bound();
    double upper = model_var.upper_bound();
    value = std::clamp(value, lower, upper);
    p_ctx.m_var_current_value[var_idx] = value;
    assert(model_var.in_bound(value));
  }
  reset_weights(p_ctx);
}

void Restart::hybrid_restart(Restart_Ctx& p_ctx) const
{
  if (!p_ctx.m_shared.m_is_found_feasible)
  {
    random_restart(p_ctx);
    return;
  }
  std::uniform_real_distribution<double> probability(0.0, 1.0);
  size_t var_num = p_ctx.m_shared.m_model_manager.var_num();
  for (size_t var_idx = 0; var_idx < var_num; ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    double random_value = sample_random_value(p_ctx, model_var);
    double lower = model_var.lower_bound();
    double upper = model_var.upper_bound();
    double best_value =
        std::clamp(p_ctx.m_shared.m_var_best_value[var_idx], lower, upper);
    double value =
        probability(p_ctx.m_rng) < 0.5 ? best_value : random_value;
    p_ctx.m_var_current_value[var_idx] = value;
    assert(model_var.in_bound(value));
  }
  reset_weights(p_ctx);
}

double Restart::sample_random_value(Restart_Ctx& p_ctx,
                                    const Model_Var& p_model_var) const
{
  double lower = p_model_var.lower_bound();
  double upper = p_model_var.upper_bound();
  bool has_finite_lower = lower > k_neg_inf * 0.5;
  bool has_finite_upper = upper < k_inf * 0.5;
  double value = 0.0;
  if (p_model_var.is_fixed())
    value = lower;
  else if (p_model_var.is_binary())
  {
    std::uniform_int_distribution<int> distribution(0, 1);
    value = static_cast<double>(distribution(p_ctx.m_rng));
  }
  else if (p_model_var.is_general_integer())
  {
    if (has_finite_lower && has_finite_upper)
    {
      long long lower_int = static_cast<long long>(std::ceil(lower));
      long long upper_int = static_cast<long long>(std::floor(upper));
      assert(lower_int <= upper_int &&
             "Invalid integer bounds: lower > upper after conversion");
      std::uniform_int_distribution<long long> distribution(lower_int,
                                                            upper_int);
      value = static_cast<double>(distribution(p_ctx.m_rng));
      return std::clamp(value, lower, upper);
    }
    else if (p_ctx.m_shared.m_is_found_feasible)
      value =
          std::clamp(p_ctx.m_shared.m_var_best_value[p_model_var.idx()],
                     lower,
                     upper);
    else if (has_finite_lower && !has_finite_upper)
      value = lower;
    else if (!has_finite_lower && has_finite_upper)
      value = upper;
    else
      value = 0.0;
  }
  else
  {
    if (has_finite_lower && has_finite_upper)
    {
      std::uniform_real_distribution<double> distribution(lower, upper);
      value = distribution(p_ctx.m_rng);
    }
    else if (p_ctx.m_shared.m_is_found_feasible)
      value =
          std::clamp(p_ctx.m_shared.m_var_best_value[p_model_var.idx()],
                     lower,
                     upper);
    else if (has_finite_lower && !has_finite_upper)
      value = lower;
    else if (!has_finite_lower && has_finite_upper)
      value = upper;
    else
      value = 0.0;
  }
  return std::clamp(value, lower, upper);
}
