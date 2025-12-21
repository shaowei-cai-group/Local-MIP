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
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

Model_Manager::Model_Manager()
    : m_bound_strengthen(1), m_is_min(1), m_obj_offset(0), m_var_num(0),
      m_general_integer_num(0), m_binary_num(0), m_fixed_num(0),
      m_real_num(0), m_con_num(0), m_delete_con_num(0),
      m_delete_var_num(0), m_infer_var_num(0), m_split_eq(true)
{
}

Model_Manager::~Model_Manager()
{
  m_var_idx_to_obj_idx.clear();
  m_var_name_to_idx.clear();
  m_var_list.clear();
  m_con_list.clear();
  m_con_name_to_idx.clear();
  m_type_to_con_idx_list.clear();
  m_type_to_con_idx_set.clear();
}

size_t Model_Manager::make_var(const std::string& p_name,
                               const bool p_integrality)
{
  auto [iter, inserted] =
      m_var_name_to_idx.try_emplace(p_name, m_var_list.size());
  if (inserted)
    m_var_list.emplace_back(p_name, iter->second, p_integrality);
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
  m_type_to_con_idx_list.clear();
  m_type_to_con_idx_set.clear();
  for (size_t con_idx = 1; con_idx < m_con_num; ++con_idx)
  {
    auto& con = m_con_list[con_idx];
    if (!con.is_inferred_sat() && con.term_num() == 0 &&
        con.verify_empty_sat())
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
      m_type_to_con_idx_list[type].push_back(con_idx);
      m_type_to_con_idx_set[type].insert(con_idx);
    }
  }
  print_cons_type_summary();
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
    if (model_var.lower_bound() >
        model_var.upper_bound() + k_feas_tolerance)
    {
      printf("c infeasible variable bound: %s LB: %.15g; UB: %.15g\n",
             model_var.name().c_str(),
             model_var.lower_bound(),
             model_var.upper_bound());
      return false;
    }
    if (model_var.is_fixed())
    {
      m_fixed_num++;
      model_var.set_type(Var_Type::fixed);
    }
    else if (model_var.is_binary())
    {
      m_binary_num++;
      model_var.set_type(Var_Type::binary);
      m_binary_idx_list.push_back(var_idx);
    }
    else if (model_var.type() == Var_Type::general_integer)
      m_general_integer_num++;
    else
    {
      model_var.set_type(Var_Type::real);
      m_real_num++;
    }
    if (!model_var.is_fixed())
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
      if (model_con.verify_empty_sat())
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
  if (std::fabs(coeff) <= k_zero_tolerance)
  {
    if (model_con.is_equality())
    {
      if (std::fabs(model_con.rhs()) > k_feas_tolerance)
      {
        printf("c tightening bound failed due to zero coefficient "
               "equality: %s, rhs: %lf\n",
               model_con.name().c_str(),
               model_con.rhs());
        return false;
      }
      return true;
    }
    if (model_con.rhs() + k_feas_tolerance < 0.0)
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
  if (var.is_fixed())
  {
    double fixed_value =
        std::midpoint(var.lower_bound(), var.upper_bound());
    if (model_con.is_equality())
    {
      double target_value = model_con.rhs() / coeff;
      if (std::fabs(target_value - fixed_value) > k_feas_tolerance)
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
    double new_bound = (model_con.rhs() + k_feas_tolerance) / coeff;
    if ((coeff > 0 && fixed_value > new_bound + k_feas_tolerance) ||
        (coeff < 0 && fixed_value < new_bound - k_feas_tolerance))
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
    if (new_bound > var.upper_bound() + k_feas_tolerance ||
        new_bound < var.lower_bound() - k_feas_tolerance)
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
      var.set_upper_bound((model_con.rhs() + k_feas_tolerance) / coeff);
      var.set_lower_bound((model_con.rhs() - k_feas_tolerance) / coeff);
    }
    else
    {
      var.set_upper_bound((model_con.rhs() - k_feas_tolerance) / coeff);
      var.set_lower_bound((model_con.rhs() + k_feas_tolerance) / coeff);
    }
  }
  else
  {
    double new_bound = (model_con.rhs() + k_feas_tolerance) / coeff;
    if ((coeff > 0 && new_bound < var.lower_bound() - k_feas_tolerance) ||
        (coeff < 0 && new_bound > var.upper_bound() + k_feas_tolerance))
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
      var.set_upper_bound(new_bound);
    else if (coeff < 0 && var.lower_bound() < new_bound) // x >= bound
      var.set_lower_bound(new_bound);
  }
  return true;
}

bool Model_Manager::global_propagation()
{
  std::vector<size_t> fixed_idxs;
  for (auto& model_var : m_var_list)
    if (model_var.is_fixed())
    {
      model_var.set_type(Var_Type::fixed);
      fixed_idxs.push_back(model_var.idx());
    }
  while (fixed_idxs.size() > 0)
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
              related_var.is_fixed())
          {
            related_var.set_type(Var_Type::fixed);
            fixed_idxs.push_back(related_var.idx());
            m_infer_var_num++;
          }
        }
        else if (model_con.term_num() == 0)
        {
          if (model_con.verify_empty_sat())
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
    Model_Var& var = m_var_list[var_idx];
    const double coeff = -p_source.coeff(term_idx);
    var.add_con(new_con_idx, new_con.term_num());
    new_con.add_var(var_idx, coeff, var.term_num() - 1);
  }
}

std::string Model_Manager::make_duplicate_constraint_name(
    const std::string& p_base) const
{
  const std::string suffix = "_linpeng";
  std::string candidate = p_base + suffix;
  size_t counter = 1;
  while (m_con_name_to_idx.find(candidate) != m_con_name_to_idx.end())
  {
    candidate = p_base + suffix + std::to_string(counter);
    counter++;
  }
  return candidate;
}

void Model_Manager::print_cons_type_summary() const
{
  static const std::vector<Con_Type> k_con_type_order = {
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
  std::vector<std::pair<std::string, size_t>> entries;
  entries.reserve(k_con_type_order.size());
  for (Con_Type type : k_con_type_order)
  {
    auto iter = m_type_to_con_idx_list.find(type);
    size_t count =
        iter == m_type_to_con_idx_list.end() ? 0 : iter->second.size();
    if (count == 0)
      continue;
    entries.emplace_back(con_type_str(type), count);
  }
  if (entries.empty())
    return;
  const std::string header_label = "Con Type";
  const std::string count_label = "Con Count";
  std::vector<std::string> type_names;
  type_names.reserve(entries.size());
  std::vector<std::string> count_values;
  count_values.reserve(entries.size());
  std::vector<size_t> column_widths(entries.size() + 1, 0);
  column_widths[0] = std::max(header_label.size(), count_label.size());
  for (size_t idx = 0; idx < entries.size(); ++idx)
  {
    type_names.emplace_back(entries[idx].first);
    count_values.emplace_back(std::to_string(entries[idx].second));
    column_widths[idx + 1] =
        std::max(type_names.back().size(), count_values.back().size());
  }
  auto print_border = [&column_widths]()
  {
    printf("c ");
    for (size_t idx = 0; idx < column_widths.size(); ++idx)
    {
      printf("+");
      for (size_t dash = 0; dash < column_widths[idx] + 2; ++dash)
        printf("-");
    }
    printf("+\n");
  };
  print_border();
  printf("c | %-*s ",
         static_cast<int>(column_widths[0]),
         header_label.c_str());
  for (size_t idx = 0; idx < type_names.size(); ++idx)
  {
    printf("| %-*s ",
           static_cast<int>(column_widths[idx + 1]),
           type_names[idx].c_str());
  }
  printf("|\n");
  print_border();
  printf("c | %-*s ",
         static_cast<int>(column_widths[0]),
         count_label.c_str());
  for (size_t idx = 0; idx < count_values.size(); ++idx)
  {
    printf("| %-*s ",
           static_cast<int>(column_widths[idx + 1]),
           count_values[idx].c_str());
  }
  printf("|\n");
  print_border();
}

void Model_Manager::classify_con(Model_Con& p_con)
{
  const size_t term_count = p_con.term_num();
  const auto& coeffs = p_con.coeff_set();
  const auto& var_idx_set = p_con.var_idx_set();
  const double rhs = p_con.rhs();
  bool is_eq = p_con.is_equality();
  bool is_leq = !is_eq;

  auto mark_type = [&](Con_Type type) { p_con.add_type(type); };

  auto is_integral_value = [&](double value)
  { return std::fabs(value - std::round(value)) <= k_zero_tolerance; };

  auto is_all_unit_coeffs = [&](const std::vector<double>& coeffs)
  {
    if (coeffs.empty())
      return false;
    for (double coeff : coeffs)
      if (std::fabs(coeff - 1.0) > k_zero_tolerance)
        return false;
    return true;
  };
  bool l_is_all_unit_coeffs = is_all_unit_coeffs(coeffs);

  auto is_all_neg_unit_coeffs = [&](const std::vector<double>& coeffs)
  {
    if (coeffs.empty())
      return false;
    for (double coeff : coeffs)
      if (std::fabs(coeff + 1.0) > k_zero_tolerance)
        return false;
    return true;
  };
  bool l_is_all_neg_unit_coeffs = is_all_neg_unit_coeffs(coeffs);

  auto has_coeff_equal_to =
      [&](const std::vector<double>& coeffs, double target)
  {
    for (double coeff : coeffs)
      if (std::fabs(coeff - target) <= k_zero_tolerance)
        return true;
    return false;
  };

  struct VarTypeFlags
  {
    bool all_binary = true;
    bool all_integral = true;
    bool has_binary = false;
    bool has_real = false;
    bool has_general_integer = false;
  };

  auto analyze_var_types = [&]() -> VarTypeFlags
  {
    VarTypeFlags flags;
    if (term_count == 0)
    {
      flags.all_binary = false;
      flags.all_integral = false;
      return flags;
    }
    for (size_t idx = 0; idx < term_count; ++idx)
    {
      const auto& var = m_var_list[var_idx_set[idx]];
      const bool is_bin = var.is_binary();
      const bool is_real = var.is_real();
      const bool is_int = var.is_general_integer();
      flags.all_binary &= is_bin;
      flags.all_integral &= !is_real;
      flags.has_binary |= is_bin;
      flags.has_real |= is_real;
      flags.has_general_integer |= (!is_bin && is_int);
    }
    return flags;
  };
  const VarTypeFlags var_flags = analyze_var_types();
  const bool l_all_binary_variables = var_flags.all_binary;
  const bool l_all_integral_variables = var_flags.all_integral;
  const bool l_has_binary_variable = var_flags.has_binary;
  const bool l_has_real_variable = var_flags.has_real;
  const bool l_has_general_integer_variable =
      var_flags.has_general_integer;
  auto classify_empty = [&]()
  {
    if (term_count == 0)
      mark_type(Con_Type::empty);
  };

  auto classify_free = [&]()
  {
    if (is_leq && k_inf <= rhs)
      mark_type(Con_Type::free);
    assert(k_neg_inf <= rhs);
  };

  auto classify_singleton = [&]()
  {
    if (term_count == 1)
      mark_type(Con_Type::singleton);
  };

  auto classify_aggregation = [&]()
  {
    if (is_eq && term_count == 2 &&
        std::fabs(coeffs[0]) > k_zero_tolerance &&
        std::fabs(coeffs[1]) > k_zero_tolerance)
      mark_type(Con_Type::aggregation);
  };

  auto classify_precedence = [&]()
  {
    if (is_leq && term_count == 2)
    {
      const double coeff_a = coeffs[0];
      const double coeff_b = coeffs[1];
      const auto& var_a = m_var_list[var_idx_set[0]];
      const auto& var_b = m_var_list[var_idx_set[1]];
      const double s = std::max(std::fabs(coeff_a), std::fabs(coeff_b));
      if (s > k_zero_tolerance &&
          std::fabs(std::fabs(coeff_a) - std::fabs(coeff_b)) <=
              k_zero_tolerance &&
          coeff_a * coeff_b < 0.0 && var_a.type() == var_b.type())
        mark_type(Con_Type::precedence);
    }
  };

  auto classify_var_bound = [&]()
  {
    if (is_leq && term_count == 2 && l_has_binary_variable)
      mark_type(Con_Type::var_bound);
  };

  auto classify_set_partitioning = [&]()
  {
    if (is_eq && term_count > 0 && l_all_binary_variables &&
        l_is_all_unit_coeffs && std::fabs(rhs - 1.0) <= k_zero_tolerance)
      mark_type(Con_Type::set_partitioning);
  };

  auto classify_set_packing = [&]()
  {
    if (is_leq && term_count > 0 && l_all_binary_variables &&
        l_is_all_unit_coeffs && std::fabs(rhs - 1.0) <= k_zero_tolerance)
      mark_type(Con_Type::set_packing);
  };

  auto classify_set_covering = [&]()
  {
    if (is_leq && term_count > 0 && l_all_binary_variables &&
        l_is_all_neg_unit_coeffs &&
        std::fabs(rhs + 1.0) <= k_zero_tolerance)
      mark_type(Con_Type::set_covering);
  };

  auto classify_cardinality = [&]()
  {
    if (is_eq && term_count > 0 && l_all_binary_variables &&
        l_is_all_unit_coeffs && is_integral_value(rhs) &&
        rhs >= 2.0 - k_zero_tolerance)
      mark_type(Con_Type::cardinality);
  };

  auto classify_invariant_knapsack = [&]()
  {
    if (is_leq && term_count > 0 && l_all_binary_variables &&
        l_is_all_unit_coeffs && is_integral_value(rhs) &&
        rhs >= 2.0 - k_zero_tolerance)
      mark_type(Con_Type::invariant_knapsack);
  };

  auto classify_equation_knapsack = [&]()
  {
    if (is_eq && term_count > 0 && l_all_binary_variables &&
        is_integral_value(rhs) && rhs >= 2.0 - k_zero_tolerance)
      mark_type(Con_Type::equation_knapsack);
  };

  auto classify_bin_packing = [&]()
  {
    if (is_leq && term_count > 0 && l_all_binary_variables &&
        is_integral_value(rhs) && rhs >= 2.0 - k_zero_tolerance &&
        has_coeff_equal_to(coeffs, rhs))
      mark_type(Con_Type::bin_packing);
  };

  auto classify_knapsack = [&]()
  {
    if (is_leq && term_count > 0 && l_all_binary_variables &&
        is_integral_value(rhs) && rhs >= 2.0 - k_zero_tolerance)
      mark_type(Con_Type::knapsack);
  };

  auto classify_integer_knapsack = [&]()
  {
    if (is_leq && term_count > 0 && l_all_integral_variables &&
        is_integral_value(rhs) && l_has_general_integer_variable)
      mark_type(Con_Type::integer_knapsack);
  };

  auto classify_mixed_binary = [&]()
  {
    if (term_count > 0 && l_has_binary_variable && l_has_real_variable &&
        !l_has_general_integer_variable)
      mark_type(Con_Type::mixed_binary);
  };

  auto classify_general_equality = [&]()
  {
    if (is_eq)
      mark_type(Con_Type::general_equality);
  };

  auto classify_general_inequality = [&]()
  {
    if (!is_eq)
      mark_type(Con_Type::general_inequality);
  };
  classify_empty();
  classify_free();
  classify_singleton();
  classify_aggregation();
  classify_precedence();
  classify_var_bound();
  classify_set_partitioning();
  classify_set_packing();
  classify_set_covering();
  classify_cardinality();
  classify_invariant_knapsack();
  classify_equation_knapsack();
  classify_bin_packing();
  classify_knapsack();
  classify_integer_knapsack();
  classify_mixed_binary();
  classify_general_equality();
  classify_general_inequality();
}
