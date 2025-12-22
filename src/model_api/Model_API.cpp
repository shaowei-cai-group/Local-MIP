/*=====================================================================================

    Filename:     Model_API.cpp

    Description:  Modeling API for Local-MIP - Implementation
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../model_data/Model_Manager.h"
#include "../utils/global_defs.h"
#include "../utils/solver_error.h"
#include "Model_API.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

Model_API::Model_API()
    : m_sense(Sense::minimize), m_obj_offset(0.0), m_var_num(0),
      m_con_num(0)
{
}

Model_API::~Model_API()
{
}

void Model_API::set_sense(Sense p_sense)
{
  m_sense = p_sense;
}

bool Model_API::set_obj_offset(double p_offset)
{
  m_obj_offset = p_offset;
  return true;
}

int Model_API::add_var(const std::string& p_name,
                       double p_lb,
                       double p_ub,
                       double p_cost,
                       Var_Type p_type)
{
  if (p_name.empty())
  {
    fprintf(stderr, "Error: Variable name cannot be empty\n");
    return -1;
  }
  if (m_var_name_to_idx.find(p_name) != m_var_name_to_idx.end())
  {
    fprintf(
        stderr, "Error: Variable '%s' already exists\n", p_name.c_str());
    return -1;
  }
  if (p_lb > p_ub)
  {
    fprintf(stderr,
            "Error: Variable '%s' has lower bound > upper bound\n",
            p_name.c_str());
    return -1;
  }
  int var_idx = static_cast<int>(m_vars.size());
  m_var_name_to_idx[p_name] = var_idx;
  VarData var_data;
  var_data.m_name = p_name;
  var_data.m_lb = p_lb;
  var_data.m_ub = p_ub;
  var_data.m_cost = p_cost;
  var_data.m_type = p_type;
  m_vars.push_back(var_data);
  m_var_num++;
  return var_idx;
}

bool Model_API::set_cost(int p_col, double p_cost)
{
  if (!is_valid_var_idx(p_col))
  {
    fprintf(stderr, "Error: Invalid variable index %d\n", p_col);
    return false;
  }
  m_vars[p_col].m_cost = p_cost;
  return true;
}

bool Model_API::set_cost(const std::string& p_name, double p_cost)
{
  int var_idx = get_var_idx(p_name);
  if (var_idx < 0)
  {
    fprintf(stderr, "Error: Variable '%s' not found\n", p_name.c_str());
    return false;
  }
  return set_cost(var_idx, p_cost);
}

int Model_API::add_con(double p_lb,
                       double p_ub,
                       const std::vector<int>& p_cols,
                       const std::vector<double>& p_coefs)
{
  if (p_cols.size() != p_coefs.size())
  {
    fprintf(stderr,
            "Error: Number of variables and coefficients do not match\n");
    return -1;
  }
  for (int col : p_cols)
  {
    if (!is_valid_var_idx(col))
    {
      fprintf(
          stderr, "Error: Invalid variable index %d in constraint\n", col);
      return -1;
    }
  }
  if (p_lb > p_ub + k_feas_tolerance)
  {
    fprintf(stderr,
            "Error: Constraint lower bound %g > upper bound %g\n",
            p_lb,
            p_ub);
    return -1;
  }
  ConData con_data;
  con_data.m_lb = p_lb;
  con_data.m_ub = p_ub;
  con_data.m_var_indices = p_cols;
  con_data.m_coefs = p_coefs;
  int con_idx = static_cast<int>(m_cons.size());
  m_cons.push_back(con_data);
  m_con_num++;
  return con_idx;
}

int Model_API::add_con(double p_lb,
                       double p_ub,
                       const std::vector<std::string>& p_names,
                       const std::vector<double>& p_coefs)
{
  if (p_names.size() != p_coefs.size())
  {
    fprintf(stderr,
            "Error: Number of variables and coefficients do not match\n");
    return -1;
  }
  std::vector<int> var_indices;
  for (const auto& name : p_names)
  {
    int var_idx = get_var_idx(name);
    if (var_idx < 0)
    {
      fprintf(stderr, "Error: Variable '%s' not found\n", name.c_str());
      return -1;
    }
    var_indices.push_back(var_idx);
  }
  return add_con(p_lb, p_ub, var_indices, p_coefs);
}

bool Model_API::add_var_to_con(int p_row, int p_col, double p_coef)
{
  if (!is_valid_con_idx(p_row))
  {
    fprintf(stderr, "Error: Invalid constraint index %d\n", p_row);
    return false;
  }
  if (!is_valid_var_idx(p_col))
  {
    fprintf(stderr, "Error: Invalid variable index %d\n", p_col);
    return false;
  }
  m_cons[p_row].m_var_indices.push_back(p_col);
  m_cons[p_row].m_coefs.push_back(p_coef);
  return true;
}

bool Model_API::add_var_to_con(int p_row,
                               const std::string& p_name,
                               double p_coef)
{
  int var_idx = get_var_idx(p_name);
  if (var_idx < 0)
  {
    fprintf(stderr, "Error: Variable '%s' not found\n", p_name.c_str());
    return false;
  }
  return add_var_to_con(p_row, var_idx, p_coef);
}

bool Model_API::set_integrality(int p_col, Var_Type p_type)
{
  if (!is_valid_var_idx(p_col))
  {
    fprintf(stderr, "Error: Invalid variable index %d\n", p_col);
    return false;
  }
  m_vars[p_col].m_type = p_type;
  return true;
}

bool Model_API::set_integrality(const std::string& p_name, Var_Type p_type)
{
  int var_idx = get_var_idx(p_name);
  if (var_idx < 0)
  {
    fprintf(stderr, "Error: Variable '%s' not found\n", p_name.c_str());
    return false;
  }
  return set_integrality(var_idx, p_type);
}

int Model_API::get_var_idx(const std::string& p_name) const
{
  auto it = m_var_name_to_idx.find(p_name);
  if (it == m_var_name_to_idx.end())
    return -1;
  return it->second;
}

bool Model_API::is_valid_var_idx(int p_idx) const
{
  return p_idx >= 0 && static_cast<size_t>(p_idx) < m_vars.size();
}

bool Model_API::is_valid_con_idx(int p_idx) const
{
  return p_idx >= 0 && static_cast<size_t>(p_idx) < m_cons.size();
}

void Model_API::add_vars_to_constraint(
    const ConData& con,
    size_t con_idx,
    Model_Manager& p_model_manager,
    const std::vector<size_t>& api_to_mgr_idx)
{
  auto& model_con = p_model_manager.con(con_idx);
  for (size_t j = 0; j < con.m_var_indices.size(); ++j)
  {
    int api_idx = con.m_var_indices[j];
    double coef = con.m_coefs[j];
    if (std::fabs(coef) < k_zero_tolerance)
      continue;
    size_t mgr_idx = api_to_mgr_idx[api_idx];
    auto& model_var = p_model_manager.var(mgr_idx);
    model_var.add_con(con_idx, model_con.term_num());
    model_con.add_var(mgr_idx, coef, model_var.term_num() - 1);
  }
}

void Model_API::build_model(Model_Manager& p_model_manager)
{
  if (p_model_manager.var_num() != 0 || p_model_manager.con_num() != 0)
    throw Solver_Error("Model_API::build_model: Model_Manager must be "
                       "empty; build once and "
                       "run once");
  printf("Building model with %zu variables and %zu constraints...\n",
         m_var_num,
         m_con_num);
  if (m_sense == Sense::maximize)
    p_model_manager.setup_max();
  if (m_obj_offset != 0.0)
    p_model_manager.add_obj_offset(m_obj_offset);
  size_t obj_idx = p_model_manager.make_con("");
  p_model_manager.set_obj_name("obj");
  std::vector<size_t> api_to_mgr_idx(m_vars.size());
  for (size_t i = 0; i < m_vars.size(); ++i)
  {
    const auto& var = m_vars[i];
    bool is_integer = (var.m_type == Var_Type::binary ||
                       var.m_type == Var_Type::general_integer);
    size_t var_idx = p_model_manager.make_var(var.m_name, is_integer);
    api_to_mgr_idx[i] = var_idx;
    auto& model_var = p_model_manager.var(var_idx);
    model_var.set_lower_bound(var.m_lb);
    model_var.set_upper_bound(var.m_ub);
    if (var.m_type == Var_Type::binary)
      model_var.set_type(Var_Type::binary);
    else if (var.m_type == Var_Type::general_integer)
      model_var.set_type(Var_Type::general_integer);
    else if (var.m_type == Var_Type::real)
      model_var.set_type(Var_Type::real);
    else if (var.m_type == Var_Type::fixed)
      model_var.set_type(Var_Type::fixed);
    if (std::fabs(var.m_cost) > k_zero_tolerance)
    {
      auto& obj_con = p_model_manager.con(obj_idx);
      model_var.add_con(obj_idx, obj_con.term_num());
      obj_con.add_var(var_idx, var.m_cost, model_var.term_num() - 1);
    }
  }
  for (size_t i = 0; i < m_cons.size(); ++i)
  {
    const auto& con = m_cons[i];
    if (con.m_var_indices.empty())
      throw Solver_Error(
          "Model_API::build_model: empty constraints are not supported");
    std::string con_name = "__api_c" + std::to_string(i);
    bool lb_is_inf = (con.m_lb <= k_neg_inf);
    bool ub_is_inf = (con.m_ub >= k_inf);
    if (std::abs(con.m_lb - con.m_ub) < k_feas_tolerance && !lb_is_inf &&
        !ub_is_inf)
    {
      size_t con_idx = p_model_manager.make_con(con_name, '=');
      auto& model_con = p_model_manager.con(con_idx);
      model_con.set_rhs(con.m_ub);
      add_vars_to_constraint(
          con, con_idx, p_model_manager, api_to_mgr_idx);
    }
    else
    {
      bool has_constraint = false;
      if (!ub_is_inf)
      {
        has_constraint = true;
        size_t con_idx = p_model_manager.make_con(con_name + "_ub", '<');
        auto& model_con = p_model_manager.con(con_idx);
        model_con.set_rhs(con.m_ub);
        add_vars_to_constraint(
            con, con_idx, p_model_manager, api_to_mgr_idx);
      }
      if (!lb_is_inf)
      {
        has_constraint = true;
        size_t con_idx = p_model_manager.make_con(con_name + "_lb", '>');
        auto& model_con = p_model_manager.con(con_idx);
        model_con.set_rhs(con.m_lb);
        add_vars_to_constraint(
            con, con_idx, p_model_manager, api_to_mgr_idx);
      }
      if (!has_constraint)
        fprintf(stderr,
                "Warning: Constraint %zu has both bounds infinite, "
                "skipping...\n",
                i);
    }
  }
  printf("Model built successfully.\n");
}