/*=====================================================================================

    Filename:     Model_Var.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../utils/global_defs.h"
#include "Model_Var.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>
#include <vector>

Model_Var::Model_Var(const std::string& p_name,
                     size_t p_idx,
                     bool p_requires_integrality)
    : m_name(p_name), m_idx(p_idx), m_upper_bound(k_inf),
      m_lower_bound(k_default_lower_bound), m_type(Var_Type::real),
      m_requires_integrality(p_requires_integrality)
{
  if (p_requires_integrality)
  {
    m_type = Var_Type::binary;
    m_upper_bound = k_default_integer_upper_bound;
    m_lower_bound = k_default_lower_bound;
  }
}

Model_Var::~Model_Var()
{
  m_con_idx_list.clear();
  m_pos_in_con_list.clear();
}

void Model_Var::set_type(Var_Type p_var_type)
{
  if (p_var_type == Var_Type::binary ||
      p_var_type == Var_Type::general_integer)
  {
    m_requires_integrality = true;
    normalize_integral_bounds();
    if (p_var_type == Var_Type::binary)
    {
      m_lower_bound = std::max(m_lower_bound, 0.0);
      m_upper_bound = std::min(m_upper_bound, 1.0);
    }
  }
  else if (p_var_type == Var_Type::real)
    m_requires_integrality = false;
  m_type = p_var_type;
}

void Model_Var::normalize_integral_bounds()
{
  m_lower_bound = std::ceil(m_lower_bound - k_feas_tolerance);
  m_upper_bound = std::floor(m_upper_bound + k_feas_tolerance);
}

void Model_Var::set_lower_bound(double p_lower_bound)
{
  assert(m_type != Var_Type::fixed);
  if (!m_requires_integrality)
    m_lower_bound = p_lower_bound;
  else
    m_lower_bound = std::ceil(p_lower_bound - k_feas_tolerance);
}

void Model_Var::set_upper_bound(double p_upper_bound)
{
  assert(m_type != Var_Type::fixed);
  if (!m_requires_integrality)
    m_upper_bound = p_upper_bound;
  else
    m_upper_bound = std::floor(p_upper_bound + k_feas_tolerance);
}

bool Model_Var::try_canonicalize_bounds()
{
  if (!std::isfinite(m_lower_bound) || !std::isfinite(m_upper_bound))
    return false;
  if (m_lower_bound <= m_upper_bound)
    return true;
  if (m_requires_integrality)
    return false;
  if (m_lower_bound > m_upper_bound + k_feas_tolerance)
    return false;

  const double fixed_value = std::midpoint(m_lower_bound, m_upper_bound);
  m_lower_bound = fixed_value;
  m_upper_bound = fixed_value;
  return true;
}

bool Model_Var::try_normalize_value(double& p_value) const
{
  if (!std::isfinite(p_value) || !std::isfinite(m_lower_bound) ||
      !std::isfinite(m_upper_bound) || m_lower_bound > m_upper_bound)
    return false;

  double normalized_value = p_value;
  if (m_requires_integrality)
  {
    const double rounded_value = std::round(normalized_value);
    if (std::fabs(normalized_value - rounded_value) > k_feas_tolerance)
      return false;
    normalized_value = rounded_value;
  }
  if (!in_bound(normalized_value))
    return false;
  p_value = std::clamp(normalized_value, m_lower_bound, m_upper_bound);
  return true;
}

void Model_Var::set_pos_in_con(const size_t term_idx,
                               const size_t pos_in_con)
{
  m_pos_in_con_list[term_idx] = pos_in_con;
}
