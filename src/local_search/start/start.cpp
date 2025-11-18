/*=====================================================================================

    Filename:     start.cpp

    Description:  Initial solution generation strategies
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../model_data/Model_Manager.h"
#include "../../utils/global_defs.h"
#include "../context/context.h"
#include "start.h"
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

Start::Start_Ctx::Start_Ctx(const Readonly_Ctx& p_shared,
                            std::vector<double>& p_var_values,
                            std::mt19937& p_rng)
    : m_shared(p_shared), m_var_current_value(p_var_values), m_rng(p_rng)
{
}

Start::Start()
    : m_user_cbk(nullptr), m_user_data(nullptr),
      m_default_method(Method::zero)
{
}

void Start::set_cbk(Start_Cbk p_start_cbk, void* p_user_data)
{
  m_user_cbk = std::move(p_start_cbk);
  m_user_data = p_user_data;
}

void Start::set_method(const std::string& p_method_name)
{
  std::string method = p_method_name;
  std::transform(method.begin(),
                 method.end(),
                 method.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (method.empty() || method == "zero")
    m_default_method = Method::zero;
  else if (method == "random")
    m_default_method = Method::random;
  else
  {
    printf("c unsupported start method %s, fallback to zero.\n",
           p_method_name.c_str());
    m_default_method = Method::zero;
  }
}

void Start::set_up_start_values(Start_Ctx& p_ctx) const
{
  if (m_user_cbk)
    m_user_cbk(p_ctx, m_user_data);
  else if (m_default_method == Method::random)
    random_start(p_ctx);
  else
    zero_start(p_ctx);
}

void Start::zero_start(Start_Ctx& p_ctx) const
{
  for (size_t var_idx = 0; var_idx < p_ctx.m_var_current_value.size();
       ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    if (model_var.lower_bound() > 0)
      p_ctx.m_var_current_value[var_idx] = model_var.lower_bound();
    else if (model_var.upper_bound() < 0)
      p_ctx.m_var_current_value[var_idx] = model_var.upper_bound();
    else
      p_ctx.m_var_current_value[var_idx] = 0;
    assert(model_var.in_bound(p_ctx.m_var_current_value[var_idx]));
  }
}

void Start::random_start(Start_Ctx& p_ctx) const
{
  zero_start(p_ctx);
  for (size_t var_idx = 0; var_idx < p_ctx.m_var_current_value.size();
       ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    bool is_integral_var =
        model_var.is_binary() || model_var.is_general_integer();
    bool has_finite_lower = model_var.lower_bound() > k_neg_inf * 0.5;
    bool has_finite_upper = model_var.upper_bound() < k_inf * 0.5;
    if (!is_integral_var || !has_finite_lower || !has_finite_upper)
      continue;
    long long lower =
        static_cast<long long>(std::llround(model_var.lower_bound()));
    long long upper =
        static_cast<long long>(std::llround(model_var.upper_bound()));
    if (lower > upper)
      std::swap(lower, upper);
    std::uniform_int_distribution<long long> distribution(lower, upper);
    p_ctx.m_var_current_value[var_idx] =
        static_cast<double>(distribution(p_ctx.m_rng));
    assert(model_var.in_bound(p_ctx.m_var_current_value[var_idx]));
  }
}
