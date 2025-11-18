/*=====================================================================================

    Filename:     Model_Con.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../utils/global_defs.h"
#include "Model_Con.h"
#include "Model_Manager.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

Model_Con::Model_Con(const std::string& p_name,
                     const size_t p_idx,
                     const char p_symbol)
    : m_name(p_name), m_idx(p_idx), m_is_equality(false),
      m_is_greater(false), m_rhs(0), m_mark_inferred_sat(false)
{
  if (p_symbol == '=')
    m_is_equality = true;
  else if (p_symbol == '>')
    m_is_greater = true;
  if (m_is_equality)
    m_types.push_back(Con_Type::general_equality);
  else
    m_types.push_back(Con_Type::general_inequality);
}

Model_Con::~Model_Con()
{
  m_coeff_list.clear();
  m_var_idx_list.clear();
  m_pos_in_var_list.clear();
  m_types.clear();
}

void Model_Con::convert_greater_to_less()
{
  assert(m_is_greater);
  for (double& coeff : m_coeff_list)
    coeff = -coeff;
  m_rhs = -m_rhs;
  m_is_greater = false;
}

void Model_Con::convert_equality_to_less()
{
  if (!m_is_equality)
    return;
  m_is_equality = false;
  m_is_greater = false;
  bool replaced = false;
  for (auto& type : m_types)
  {
    if (type == Con_Type::general_equality)
    {
      type = Con_Type::general_inequality;
      replaced = true;
      break;
    }
  }
  assert(replaced);
  if (!replaced)
    m_types.push_back(Con_Type::general_inequality);
}

void Model_Con::delete_term_at(const size_t p_term_idx,
                               double p_delete_var_value,
                               Model_Manager* p_model_manager)
{
  assert(p_term_idx < term_num());
  double delete_coeff = m_coeff_list[p_term_idx];
  size_t moved_var_idx = m_var_idx_list.back();
  double moved_coeff = m_coeff_list.back();
  size_t moved_pos_in_var = m_pos_in_var_list.back();
  m_var_idx_list[p_term_idx] = moved_var_idx;
  m_coeff_list[p_term_idx] = moved_coeff;
  m_pos_in_var_list[p_term_idx] = moved_pos_in_var;
  m_var_idx_list.pop_back();
  m_coeff_list.pop_back();
  m_pos_in_var_list.pop_back();
  auto& moved_var = p_model_manager->var(moved_var_idx);
  assert(moved_var.pos_in_con(moved_pos_in_var) == term_num());
  assert(moved_var.con_idx(moved_pos_in_var) == m_idx);
  moved_var.set_pos_in_con(moved_pos_in_var, p_term_idx);
  if (m_idx == 0)
    p_model_manager->add_obj_offset(delete_coeff * p_delete_var_value);
  else
    m_rhs -= delete_coeff * p_delete_var_value;
}
