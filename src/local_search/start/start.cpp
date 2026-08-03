/*=====================================================================================

    Filename:     start.cpp

    Description:  Initial solution generation strategies
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../../model_data/Model_Manager.h"
#include "../../utils/global_defs.h"
#include "../../utils/solver_error.h"
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
  else if (method == "objective")
    m_default_method = Method::objective_guided;
  else if (method == "locks")
    m_default_method = Method::lock_guided;
  else
  {
    printf("c unsupported start method %s, fallback to zero.\n",
           p_method_name.c_str());
    m_default_method = Method::zero;
  }
}

void Start::set_up_start_values(
    Start_Ctx& p_ctx,
    const std::vector<double>& p_start_solution,
    const std::vector<char>& p_start_mask) const
{
  if (!p_start_solution.empty())
  {
    if (p_start_solution.size() != p_ctx.m_var_current_value.size())
    {
      throw Solver_Error(
          "start solution size does not match variable count: " +
          std::to_string(p_start_solution.size()) +
          " != " + std::to_string(p_ctx.m_var_current_value.size()));
    }
    if (!p_start_mask.empty() &&
        p_start_mask.size() != p_start_solution.size())
    {
      throw Solver_Error("start solution presence mask size does not "
                         "match variable count: " +
                         std::to_string(p_start_mask.size()) +
                         " != " + std::to_string(p_start_solution.size()));
    }
    if (!p_start_mask.empty())
      zero_start(p_ctx);
    for (size_t var_idx = 0; var_idx < p_start_solution.size(); ++var_idx)
      if (p_start_mask.empty() || p_start_mask[var_idx])
        p_ctx.m_var_current_value[var_idx] = p_start_solution[var_idx];
  }
  else if (m_user_cbk)
    m_user_cbk(p_ctx, m_user_data);
  else
  {
    switch (m_default_method)
    {
      case Method::random:
        random_start(p_ctx);
        break;
      case Method::objective_guided:
        objective_guided_start(p_ctx);
        break;
      case Method::lock_guided:
        lock_guided_start(p_ctx);
        break;
      case Method::zero:
        zero_start(p_ctx);
        break;
    }
  }
}

void Start::zero_start(Start_Ctx& p_ctx) const
{
  for (size_t var_idx = 0; var_idx < p_ctx.m_var_current_value.size();
       ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    p_ctx.m_var_current_value[var_idx] = closest_to_zero(model_var);
    assert(p_ctx.m_shared.m_model_manager.var_in_bound(
        model_var, p_ctx.m_var_current_value[var_idx]));
  }
}

void Start::random_start(Start_Ctx& p_ctx) const
{
  zero_start(p_ctx);
  for (size_t var_idx = 0; var_idx < p_ctx.m_var_current_value.size();
       ++var_idx)
  {
    const auto& model_var = p_ctx.m_shared.m_model_manager.var(var_idx);
    bool is_integral_var = model_var.type() == Var_Type::binary ||
                           model_var.is_general_integer();
    bool has_finite_lower = model_var.lower_bound() > k_neg_inf * 0.5;
    bool has_finite_upper = model_var.upper_bound() < k_inf * 0.5;
    if (!is_integral_var || !has_finite_lower || !has_finite_upper ||
        !fits_in_long_long(model_var.lower_bound()) ||
        !fits_in_long_long(model_var.upper_bound()))
      continue;
    long long lower = static_cast<long long>(model_var.lower_bound());
    long long upper = static_cast<long long>(model_var.upper_bound());
    if (lower > upper)
      std::swap(lower, upper);
    std::uniform_int_distribution<long long> distribution(lower, upper);
    p_ctx.m_var_current_value[var_idx] =
        static_cast<double>(distribution(p_ctx.m_rng));
    assert(p_ctx.m_shared.m_model_manager.var_in_bound(
        model_var, p_ctx.m_var_current_value[var_idx]));
  }
}

void Start::objective_guided_start(Start_Ctx& p_ctx) const
{
  assert(p_ctx.m_shared.m_var_obj_cost.size() ==
         p_ctx.m_var_current_value.size());
  for (size_t var_idx = 0; var_idx < p_ctx.m_var_current_value.size();
       ++var_idx)
  {
    p_ctx.m_var_current_value[var_idx] =
        objective_guided_value(p_ctx, var_idx);
    assert(p_ctx.m_shared.m_model_manager.var_in_bound(
        p_ctx.m_shared.m_model_manager.var(var_idx),
        p_ctx.m_var_current_value[var_idx]));
  }
}

void Start::lock_guided_start(Start_Ctx& p_ctx) const
{
  const auto& model_manager = p_ctx.m_shared.m_model_manager;
  const size_t var_num = p_ctx.m_var_current_value.size();
  assert(model_manager.var_num() == var_num);
  assert(p_ctx.m_shared.m_var_obj_cost.size() == var_num);
  const double zero_tolerance = model_manager.zero_tolerance();

  std::vector<size_t> up_locks(var_num, 0);
  std::vector<size_t> down_locks(var_num, 0);
  for (size_t con_idx = 1; con_idx < model_manager.con_num(); ++con_idx)
  {
    const auto& model_con = model_manager.con(con_idx);
    if (model_con.is_inferred_sat())
      continue;
    for (size_t term_idx = 0; term_idx < model_con.term_num(); ++term_idx)
    {
      const double coeff = model_con.coeff(term_idx);
      if (std::fabs(coeff) <= zero_tolerance)
        continue;
      const size_t var_idx = model_con.var_idx(term_idx);
      if (model_con.is_equality())
      {
        ++up_locks[var_idx];
        ++down_locks[var_idx];
      }
      else if (coeff > zero_tolerance)
        ++up_locks[var_idx];
      else
        ++down_locks[var_idx];
    }
  }

  for (size_t var_idx = 0; var_idx < var_num; ++var_idx)
  {
    const auto& model_var = model_manager.var(var_idx);
    double value;
    if (model_var.type() == Var_Type::fixed)
      value = model_var.lower_bound();
    else if (down_locks[var_idx] < up_locks[var_idx])
      value = select_bound(model_var.lower_bound(), model_var);
    else if (up_locks[var_idx] < down_locks[var_idx])
      value = select_bound(model_var.upper_bound(), model_var);
    else
      value = objective_guided_value(p_ctx, var_idx);
    p_ctx.m_var_current_value[var_idx] = value;
    assert(model_manager.var_in_bound(model_var, value));
  }
}

double Start::closest_to_zero(const Model_Var& p_model_var) const
{
  if (p_model_var.type() == Var_Type::fixed)
    return p_model_var.lower_bound();
  if (p_model_var.lower_bound() > 0.0)
    return p_model_var.lower_bound();
  if (p_model_var.upper_bound() < 0.0)
    return p_model_var.upper_bound();
  return 0.0;
}

double Start::select_bound(double p_preferred_bound,
                           const Model_Var& p_model_var) const
{
  if (p_model_var.type() == Var_Type::fixed)
    return p_model_var.lower_bound();
  if (p_preferred_bound > k_neg_inf && p_preferred_bound < k_inf)
    return p_preferred_bound;
  return closest_to_zero(p_model_var);
}

double Start::objective_guided_value(const Start_Ctx& p_ctx,
                                     size_t p_var_idx) const
{
  const auto& model_var = p_ctx.m_shared.m_model_manager.var(p_var_idx);
  const auto& model_manager = p_ctx.m_shared.m_model_manager;
  if (model_var.type() == Var_Type::fixed)
    return model_var.lower_bound();
  const double obj_coeff = p_ctx.m_shared.m_var_obj_cost[p_var_idx];
  const double zero_tolerance = model_manager.zero_tolerance();
  if (obj_coeff > zero_tolerance)
    return select_bound(model_var.lower_bound(), model_var);
  if (obj_coeff < -zero_tolerance)
    return select_bound(model_var.upper_bound(), model_var);
  return closest_to_zero(model_var);
}
