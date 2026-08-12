/*=====================================================================================

    Filename:     Model_Manager.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../utils/global_defs.h"
#include "Model_Con.h"
#include "Model_Manager.h"
#include "Model_Var.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

Model_Manager::Model_Manager(double p_feas_tolerance,
                             double p_zero_tolerance)
    : m_bound_strengthen(1), m_feas_tolerance(p_feas_tolerance),
      m_zero_tolerance(p_zero_tolerance), m_is_min(1), m_obj_offset(0),
      m_var_num(0), m_general_integer_num(0), m_binary_num(0),
      m_fixed_num(0), m_real_num(0), m_con_num(0), m_delete_con_num(0),
      m_delete_var_num(0), m_infer_var_num(0), m_split_eq(true)
{
}

Model_Manager::~Model_Manager() = default;

size_t Model_Manager::make_var(const std::string& p_name,
                               bool p_requires_integrality)
{
  auto [iter, inserted] =
      m_var_name_to_idx.try_emplace(p_name, m_var_list.size());
  if (inserted)
    m_var_list.emplace_back(p_name, iter->second, p_requires_integrality);
  return iter->second;
}

size_t Model_Manager::make_con(const std::string& p_name,
                               const char p_type)
{
  auto [iter, inserted] =
      m_con_name_to_idx.try_emplace(p_name, m_con_list.size());
  if (inserted)
    m_con_list.emplace_back(p_name, iter->second, p_type);
  return iter->second;
}

void Model_Manager::add_term(size_t p_con_idx,
                             size_t p_var_idx,
                             double p_coeff)
{
  Model_Con& model_con = con(p_con_idx);
  Model_Var& model_var = var(p_var_idx);
  model_var.add_con(p_con_idx, model_con.term_num());
  model_con.add_var(p_var_idx, p_coeff, model_var.term_num() - 1);
}

void Model_Manager::normalize_integral_bounds(Model_Var& p_var) const
{
  p_var.m_lower_bound = std::ceil(p_var.m_lower_bound - m_feas_tolerance);
  p_var.m_upper_bound = std::floor(p_var.m_upper_bound + m_feas_tolerance);
}

void Model_Manager::set_var_type(Model_Var& p_var, Var_Type p_type)
{
  if (p_type == Var_Type::binary || p_type == Var_Type::general_integer)
  {
    p_var.m_requires_integrality = true;
    normalize_integral_bounds(p_var);
    if (p_type == Var_Type::binary)
    {
      p_var.m_lower_bound = std::max(p_var.m_lower_bound, 0.0);
      p_var.m_upper_bound = std::min(p_var.m_upper_bound, 1.0);
    }
  }
  else if (p_type == Var_Type::real)
    p_var.m_requires_integrality = false;
  p_var.m_type = p_type;
}

void Model_Manager::set_var_lower_bound(Model_Var& p_var,
                                        double p_lower_bound)
{
  assert(p_var.m_type != Var_Type::fixed);
  if (p_var.m_requires_integrality)
    p_var.m_lower_bound = std::ceil(p_lower_bound - m_feas_tolerance);
  else
    p_var.m_lower_bound = p_lower_bound;
}

void Model_Manager::set_var_upper_bound(Model_Var& p_var,
                                        double p_upper_bound)
{
  assert(p_var.m_type != Var_Type::fixed);
  if (p_var.m_requires_integrality)
    p_var.m_upper_bound = std::floor(p_upper_bound + m_feas_tolerance);
  else
    p_var.m_upper_bound = p_upper_bound;
}

bool Model_Manager::canonicalize_var_bounds(Model_Var& p_var) const
{
  if (!std::isfinite(p_var.m_lower_bound) ||
      !std::isfinite(p_var.m_upper_bound))
    return false;
  if (p_var.m_lower_bound <= p_var.m_upper_bound)
    return true;
  if (p_var.m_requires_integrality ||
      p_var.m_lower_bound > p_var.m_upper_bound + m_feas_tolerance)
    return false;

  const double fixed_value =
      std::midpoint(p_var.m_lower_bound, p_var.m_upper_bound);
  p_var.m_lower_bound = fixed_value;
  p_var.m_upper_bound = fixed_value;
  return true;
}

bool Model_Manager::var_is_fixed(const Model_Var& p_var) const
{
  return is_effectively_zero(p_var.lower_bound() - p_var.upper_bound(),
                             m_feas_tolerance);
}

bool Model_Manager::var_is_binary(const Model_Var& p_var) const
{
  return p_var.type() == Var_Type::binary ||
         (p_var.type() == Var_Type::general_integer &&
          is_effectively_zero(p_var.lower_bound(), m_feas_tolerance) &&
          is_effectively_zero(p_var.upper_bound() - 1.0,
                              m_feas_tolerance));
}

bool Model_Manager::empty_con_is_satisfied(const Model_Con& p_con) const
{
  return (!p_con.is_equality() && p_con.rhs() + m_feas_tolerance >= 0.0) ||
         (p_con.is_equality() &&
          std::fabs(p_con.rhs()) <= m_feas_tolerance);
}

bool Model_Manager::var_in_bound(const Model_Var& p_var,
                                 double p_value) const
{
  return p_var.m_lower_bound - m_feas_tolerance <= p_value &&
         p_value <= p_var.m_upper_bound + m_feas_tolerance;
}

bool Model_Manager::normalize_var_value(const Model_Var& p_var,
                                        double& p_value) const
{
  if (!std::isfinite(p_value) || !std::isfinite(p_var.m_lower_bound) ||
      !std::isfinite(p_var.m_upper_bound) ||
      p_var.m_lower_bound > p_var.m_upper_bound)
    return false;

  double normalized_value = p_value;
  if (p_var.m_requires_integrality)
  {
    const double rounded_value = std::round(normalized_value);
    if (std::fabs(normalized_value - rounded_value) > m_feas_tolerance)
      return false;
    normalized_value = rounded_value;
  }
  if (!var_in_bound(p_var, normalized_value))
    return false;
  p_value = std::clamp(
      normalized_value, p_var.m_lower_bound, p_var.m_upper_bound);
  return true;
}

bool Model_Manager::process_after_read()
{
  m_var_num = m_var_list.size();
  const size_t original_con_num = m_con_list.size();
  printf("c original problem has %zu variables and %zu constraints\n",
         m_var_num,
         original_con_num - 1);
  if (m_split_eq)
    convert_eq_to_ineq();
  m_con_num = m_con_list.size();
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    auto& con = m_con_list[con_idx];
    if (con.is_greater())
      con.convert_greater_to_less();
  }
  if (!m_con_list.empty() && m_obj_offset == 0.0)
    m_obj_offset = -m_con_list[0].rhs();
  if (!m_con_list.empty() && m_is_min == -1)
  {
    auto& obj_con = m_con_list[0];
    for (size_t i = 0; i < obj_con.term_num(); ++i)
      obj_con.set_coeff(i, -obj_con.coeff(i));
    m_obj_offset = -m_obj_offset;
  }
  if (!calculate_vars())
  {
    printf("c model is infeasible due to variable bounds.\n");
    return false;
  }
  if ((m_bound_strengthen == 1 && m_real_num == 0) ||
      m_bound_strengthen == 2)
    if (!tighten_bounds() || !global_propagation() || !calculate_vars())
    {
      printf("c model is infeasible after bound tightening.\n");
      return false;
    }
  std::unordered_map<Con_Type, size_t> con_type_counts;
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    auto& con = m_con_list[con_idx];
    if (!con.is_inferred_sat() && con.term_num() == 0 &&
        empty_con_is_satisfied(con))
    {
      con.mark_inferred_sat();
      m_delete_con_num++;
    }
    classify_con(con);
    if (con.is_inferred_sat())
      continue;
    const auto& types = con.get_types();
    for (Con_Type type : types)
    {
      ++con_type_counts[type];
    }
  }
  print_cons_type_summary(con_type_counts);
  m_var_idx_to_obj_idx.resize(m_var_num, SIZE_MAX);
  m_var_obj_cost.resize(m_var_num, 0.0);
  const auto& model_obj = obj();
  for (size_t term_idx = 0; term_idx < model_obj.term_num(); ++term_idx)
  {
    size_t var_idx = model_obj.var_idx(term_idx);
    m_var_obj_cost[var_idx] = model_obj.coeff(term_idx);
    m_var_idx_to_obj_idx[var_idx] = term_idx;
  }
  m_con_is_equality.resize(m_con_num, false);
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
    m_con_is_equality[con_idx] = m_con_list[con_idx].is_equality();
  return true;
}

bool Model_Manager::calculate_vars()
{
  m_general_integer_num = 0;
  m_binary_num = 0;
  m_fixed_num = 0;
  m_real_num = 0;
  m_binary_idx_list.clear();
  m_non_fixed_var_idxs.clear();
  m_binary_idx_list.reserve(m_var_num);
  m_non_fixed_var_idxs.reserve(m_var_num);
  for (size_t var_idx = 0; var_idx < m_var_num; var_idx++)
  {
    auto& model_var = m_var_list[var_idx];
    if (!canonicalize_var_bounds(model_var))
    {
      printf("c infeasible variable bound: %s LB: %.15g; UB: %.15g\n",
             model_var.name().c_str(),
             model_var.lower_bound(),
             model_var.upper_bound());
      return false;
    }
    if (var_is_fixed(model_var))
    {
      m_fixed_num++;
      set_var_type(model_var, Var_Type::fixed);
    }
    else if (var_is_binary(model_var))
    {
      m_binary_num++;
      set_var_type(model_var, Var_Type::binary);
      m_binary_idx_list.push_back(var_idx);
    }
    else if (model_var.type() == Var_Type::general_integer)
      m_general_integer_num++;
    else
    {
      set_var_type(model_var, Var_Type::real);
      m_real_num++;
    }
    if (!var_is_fixed(model_var))
      m_non_fixed_var_idxs.push_back(var_idx);
  }
  printf("c fixed: %zu, binary: %zu, general integer: %zu, real: %zu\n",
         m_fixed_num,
         m_binary_num,
         m_general_integer_num,
         m_real_num);
  return true;
}

bool Model_Manager::tighten_bounds()
{
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    auto& model_con = m_con_list[con_idx];
    if (model_con.term_num() == 1)
    {
      if (!singleton_deduction(model_con))
        return false;
      model_con.mark_inferred_sat();
      m_delete_con_num++;
    }
    if (model_con.term_num() == 0)
    {
      if (empty_con_is_satisfied(model_con))
      {
        model_con.mark_inferred_sat();
        m_delete_con_num++;
      }
      else
      {
        printf("c tightening bound failed due to empty constraint: %s, "
               "rhs: %lf\n",
               model_con.name().c_str(),
               model_con.rhs());
        return false;
      }
    }
  }
  return true;
}

bool Model_Manager::singleton_deduction(Model_Con& model_con)
{
  double coeff = model_con.unique_coeff();
  if (std::fabs(coeff) <= m_zero_tolerance)
  {
    if (model_con.is_equality())
    {
      if (std::fabs(model_con.rhs()) > m_feas_tolerance)
      {
        printf("c tightening bound failed due to zero coefficient "
               "equality: %s, rhs: %lf\n",
               model_con.name().c_str(),
               model_con.rhs());
        return false;
      }
      return true;
    }
    if (model_con.rhs() + m_feas_tolerance < 0.0)
    {
      printf("c tightening bound failed due to zero coefficient "
             "inequality: %s, rhs: %lf\n",
             model_con.name().c_str(),
             model_con.rhs());
      return false;
    }
    return true;
  }
  auto& var = m_var_list[model_con.unique_var_idx()];
  if (var_is_fixed(var))
  {
    double fixed_value =
        std::midpoint(var.lower_bound(), var.upper_bound());
    if (model_con.is_equality())
    {
      double target_value = model_con.rhs() / coeff;
      if (std::fabs(target_value - fixed_value) > m_feas_tolerance)
      {
        printf("c tightening bound failed due to equality constraint: "
               "%s, rhs: %lf, coeff: %lf, fixed_value: %lf, "
               "upper_bound: %lf, lower_bound: %lf\n",
               model_con.name().c_str(),
               model_con.rhs(),
               coeff,
               fixed_value,
               var.upper_bound(),
               var.lower_bound());
        return false;
      }
      return true;
    }
    double new_bound = (model_con.rhs() + m_feas_tolerance) / coeff;
    if ((coeff > 0 && fixed_value > new_bound + m_feas_tolerance) ||
        (coeff < 0 && fixed_value < new_bound - m_feas_tolerance))
    {
      printf("c tightening bound failed due to inequality "
             "constraint: %s, rhs: %lf, coeff: %lf, new_bound: %lf, "
             "fixed_value: %lf\n",
             model_con.name().c_str(),
             model_con.rhs(),
             coeff,
             new_bound,
             fixed_value);
      return false;
    }
    return true;
  }
  if (model_con.is_equality())
  {
    double new_bound = model_con.rhs() / coeff;
    if (new_bound > var.upper_bound() + m_feas_tolerance ||
        new_bound < var.lower_bound() - m_feas_tolerance)
    {
      printf("c tightening bound failed due to equality constraint: "
             "%s, rhs: %lf, coeff: %lf, new_bound: %lf, "
             "upper_bound: %lf, lower_bound: %lf\n",
             model_con.name().c_str(),
             model_con.rhs(),
             coeff,
             new_bound,
             var.upper_bound(),
             var.lower_bound());
      return false;
    }
    if (coeff > 0)
    {
      set_var_upper_bound(var,
                          (model_con.rhs() + m_feas_tolerance) / coeff);
      set_var_lower_bound(var,
                          (model_con.rhs() - m_feas_tolerance) / coeff);
    }
    else
    {
      set_var_upper_bound(var,
                          (model_con.rhs() - m_feas_tolerance) / coeff);
      set_var_lower_bound(var,
                          (model_con.rhs() + m_feas_tolerance) / coeff);
    }
  }
  else
  {
    double new_bound = (model_con.rhs() + m_feas_tolerance) / coeff;
    if ((coeff > 0 && new_bound < var.lower_bound() - m_feas_tolerance) ||
        (coeff < 0 && new_bound > var.upper_bound() + m_feas_tolerance))
    {
      printf("c tightening bound failed due to inequality "
             "constraint: %s, rhs: %lf, coeff: %lf, new_bound: %lf, "
             "upper_bound: %lf, lower_bound: %lf\n",
             model_con.name().c_str(),
             model_con.rhs(),
             coeff,
             new_bound,
             var.upper_bound(),
             var.lower_bound());
      return false;
    }
    if (coeff > 0 && new_bound < var.upper_bound()) // x <= bound
      set_var_upper_bound(var, new_bound);
    else if (coeff < 0 && var.lower_bound() < new_bound) // x >= bound
      set_var_lower_bound(var, new_bound);
  }
  return true;
}

bool Model_Manager::global_propagation()
{
  std::vector<size_t> fixed_idxs;
  for (auto& model_var : m_var_list)
    if (var_is_fixed(model_var))
    {
      set_var_type(model_var, Var_Type::fixed);
      fixed_idxs.push_back(model_var.idx());
    }
  while (!fixed_idxs.empty())
  {
    size_t delete_var_idx = fixed_idxs.back();
    fixed_idxs.pop_back();
    m_delete_var_num++;
    Model_Var& delete_var = m_var_list[delete_var_idx];
    double delete_var_value =
        std::midpoint(delete_var.lower_bound(), delete_var.upper_bound());
    for (size_t term_idx = 0; term_idx < delete_var.term_num(); term_idx++)
    {
      size_t con_idx = delete_var.con_idx(term_idx);
      size_t pos_in_con = delete_var.pos_in_con(term_idx);
      Model_Con& model_con = m_con_list[con_idx];
      model_con.delete_term_at(pos_in_con, delete_var_value, this);
      if (con_idx != 0)
      {
        if (model_con.term_num() == 1)
        {
          if (!singleton_deduction(model_con))
            return false;
          model_con.mark_inferred_sat();
          m_delete_con_num++;
          Model_Var& related_var = m_var_list[model_con.unique_var_idx()];
          if (related_var.type() != Var_Type::fixed &&
              var_is_fixed(related_var))
          {
            set_var_type(related_var, Var_Type::fixed);
            fixed_idxs.push_back(related_var.idx());
            m_infer_var_num++;
          }
        }
        else if (model_con.term_num() == 0)
        {
          if (empty_con_is_satisfied(model_con))
          {
            model_con.mark_inferred_sat();
            m_delete_con_num++;
          }
          else
          {
            printf(
                "c tightening bound failed due to empty constraint: %s, "
                "rhs: %lf\n",
                model_con.name().c_str(),
                model_con.rhs());
            return false;
          }
        }
      }
    }
  }
  printf("c delete con num: %zu\n", m_delete_con_num);
  printf("c delete var num: %zu\n", m_delete_var_num);
  printf("c infer var num: %zu\n", m_infer_var_num);
  return true;
}

void Model_Manager::convert_eq_to_ineq()
{
  const size_t original_con_num = m_con_list.size();
  size_t equality_count = 0;
  for (size_t con_idx = 1; con_idx < original_con_num; ++con_idx)
  {
    if (m_con_list[con_idx].is_equality())
      equality_count++;
  }
  if (equality_count == 0)
    return;
  m_con_list.reserve(m_con_list.size() + equality_count);
  for (size_t con_idx = 1; con_idx < original_con_num; ++con_idx)
  {
    Model_Con& con = m_con_list[con_idx];
    if (!con.is_equality())
      continue;
    con.convert_equality_to_less();
    append_negated_con(con);
  }
  printf(
      "c converted %zu equality constraints to inequality constraints\n",
      equality_count);
}

void Model_Manager::append_negated_con(const Model_Con& p_source)
{
  const size_t new_con_idx = m_con_list.size();
  std::string new_name = make_duplicate_constraint_name(p_source.name());
  m_con_list.emplace_back(new_name, new_con_idx, '<');
  m_con_name_to_idx.emplace(new_name, new_con_idx);
  Model_Con& new_con = m_con_list.back();
  new_con.set_rhs(-p_source.rhs());
  const size_t term_num = p_source.term_num();
  for (size_t term_idx = 0; term_idx < term_num; ++term_idx)
  {
    const size_t var_idx = p_source.var_idx(term_idx);
    const double coeff = -p_source.coeff(term_idx);
    add_term(new_con_idx, var_idx, coeff);
  }
}

std::string Model_Manager::make_duplicate_constraint_name(
    const std::string& p_base) const
{
  const std::string suffix = "_linpeng";
  std::string candidate = p_base + suffix;
  size_t counter = 1;
  while (m_con_name_to_idx.contains(candidate))
  {
    candidate = p_base + suffix + std::to_string(counter);
    counter++;
  }
  return candidate;
}

void Model_Manager::print_cons_type_summary(
    const std::unordered_map<Con_Type, size_t>& p_type_counts) const
{
  static constexpr std::array k_con_type_order = {
      Con_Type::empty,
      Con_Type::free,
      Con_Type::singleton,
      Con_Type::aggregation,
      Con_Type::precedence,
      Con_Type::var_bound,
      Con_Type::set_partitioning,
      Con_Type::set_packing,
      Con_Type::set_covering,
      Con_Type::cardinality,
      Con_Type::invariant_knapsack,
      Con_Type::equation_knapsack,
      Con_Type::bin_packing,
      Con_Type::knapsack,
      Con_Type::integer_knapsack,
      Con_Type::mixed_binary,
      Con_Type::general_equality,
      Con_Type::general_inequality};
  struct Column
  {
    std::string type;
    std::string count;
    size_t width;
  };
  std::vector<Column> columns;
  columns.reserve(k_con_type_order.size());
  for (Con_Type type : k_con_type_order)
  {
    const auto iter = p_type_counts.find(type);
    const size_t count = iter == p_type_counts.end() ? 0 : iter->second;
    if (count == 0)
      continue;
    std::string type_name = con_type_str(type);
    std::string count_value = std::to_string(count);
    const size_t width = std::max(type_name.size(), count_value.size());
    columns.push_back(
        {std::move(type_name), std::move(count_value), width});
  }
  if (columns.empty())
    return;
  const std::string header_label = "Con Type";
  const std::string count_label = "Con Count";
  const size_t label_width =
      std::max(header_label.size(), count_label.size());
  auto print_border = [&]()
  {
    printf("c ");
    auto print_column_border = [](size_t p_width)
    {
      printf("+");
      for (size_t dash = 0; dash < p_width + 2; ++dash)
        printf("-");
    };
    print_column_border(label_width);
    for (const Column& column : columns)
      print_column_border(column.width);
    printf("+\n");
  };
  print_border();
  printf("c | %-*s ", static_cast<int>(label_width), header_label.c_str());
  for (const Column& column : columns)
  {
    printf("| %-*s ", static_cast<int>(column.width), column.type.c_str());
  }
  printf("|\n");
  print_border();
  printf("c | %-*s ", static_cast<int>(label_width), count_label.c_str());
  for (const Column& column : columns)
  {
    printf(
        "| %-*s ", static_cast<int>(column.width), column.count.c_str());
  }
  printf("|\n");
  print_border();
}

void Model_Manager::classify_con(Model_Con& p_con)
{
  const size_t term_count = p_con.term_num();
  const auto& coeffs = p_con.coeff_set();
  const auto& var_idxs = p_con.var_idx_set();
  const double rhs = p_con.rhs();
  const bool is_equality = p_con.is_equality();
  const bool all_unit_coeffs =
      !coeffs.empty() &&
      std::all_of(
          coeffs.begin(),
          coeffs.end(),
          [&](double p_coeff)
          { return !(std::fabs(p_coeff - 1.0) > m_zero_tolerance); });
  const bool all_neg_unit_coeffs =
      !coeffs.empty() &&
      std::all_of(
          coeffs.begin(),
          coeffs.end(),
          [&](double p_coeff)
          { return !(std::fabs(p_coeff + 1.0) > m_zero_tolerance); });
  const bool rhs_is_integral =
      std::fabs(rhs - std::round(rhs)) <= m_zero_tolerance;
  const bool has_rhs_coefficient = std::any_of(
      coeffs.begin(),
      coeffs.end(),
      [&](double p_coeff)
      { return std::fabs(p_coeff - rhs) <= m_zero_tolerance; });

  bool all_binary = term_count > 0;
  bool all_integral = term_count > 0;
  bool has_binary = false;
  bool has_real = false;
  bool has_general_integer = false;
  for (size_t var_idx : var_idxs)
  {
    const auto& var = m_var_list[var_idx];
    const bool is_binary = var_is_binary(var);
    const bool is_real = var.is_real();
    const bool is_general_integer = var.is_general_integer();
    all_binary &= is_binary;
    all_integral &= !is_real;
    has_binary |= is_binary;
    has_real |= is_real;
    has_general_integer |= !is_binary && is_general_integer;
  }

  if (term_count == 0)
    p_con.add_type(Con_Type::empty);
  if (!is_equality && k_inf <= rhs)
    p_con.add_type(Con_Type::free);
  assert(k_neg_inf <= rhs);
  if (term_count == 1)
    p_con.add_type(Con_Type::singleton);
  if (is_equality && term_count == 2 &&
      std::fabs(coeffs[0]) > m_zero_tolerance &&
      std::fabs(coeffs[1]) > m_zero_tolerance)
    p_con.add_type(Con_Type::aggregation);

  if (!is_equality && term_count == 2)
  {
    const double coeff_a = coeffs[0];
    const double coeff_b = coeffs[1];
    const auto& var_a = m_var_list[var_idxs[0]];
    const auto& var_b = m_var_list[var_idxs[1]];
    const double max_coeff =
        std::max(std::fabs(coeff_a), std::fabs(coeff_b));
    if (max_coeff > m_zero_tolerance &&
        std::fabs(std::fabs(coeff_a) - std::fabs(coeff_b)) <=
            m_zero_tolerance &&
        coeff_a * coeff_b < 0.0 && var_a.type() == var_b.type())
      p_con.add_type(Con_Type::precedence);
  }

  if (!is_equality && term_count == 2 && has_binary)
    p_con.add_type(Con_Type::var_bound);
  if (is_equality && all_binary && all_unit_coeffs &&
      std::fabs(rhs - 1.0) <= m_zero_tolerance)
    p_con.add_type(Con_Type::set_partitioning);
  if (!is_equality && all_binary && all_unit_coeffs &&
      std::fabs(rhs - 1.0) <= m_zero_tolerance)
    p_con.add_type(Con_Type::set_packing);
  if (!is_equality && all_binary && all_neg_unit_coeffs &&
      std::fabs(rhs + 1.0) <= m_zero_tolerance)
    p_con.add_type(Con_Type::set_covering);
  if (is_equality && all_binary && all_unit_coeffs && rhs_is_integral &&
      rhs >= 2.0 - m_zero_tolerance)
    p_con.add_type(Con_Type::cardinality);
  if (!is_equality && all_binary && all_unit_coeffs && rhs_is_integral &&
      rhs >= 2.0 - m_zero_tolerance)
    p_con.add_type(Con_Type::invariant_knapsack);
  if (is_equality && all_binary && rhs_is_integral &&
      rhs >= 2.0 - m_zero_tolerance)
    p_con.add_type(Con_Type::equation_knapsack);
  if (!is_equality && all_binary && rhs_is_integral &&
      rhs >= 2.0 - m_zero_tolerance && has_rhs_coefficient)
    p_con.add_type(Con_Type::bin_packing);
  if (!is_equality && all_binary && rhs_is_integral &&
      rhs >= 2.0 - m_zero_tolerance)
    p_con.add_type(Con_Type::knapsack);
  if (!is_equality && all_integral && rhs_is_integral &&
      has_general_integer)
    p_con.add_type(Con_Type::integer_knapsack);
  if (has_binary && has_real && !has_general_integer)
    p_con.add_type(Con_Type::mixed_binary);
  p_con.add_type(is_equality ? Con_Type::general_equality
                             : Con_Type::general_inequality);
}
