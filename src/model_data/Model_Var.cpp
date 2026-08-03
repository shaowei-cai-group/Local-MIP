/*=====================================================================================

    Filename:     Model_Var.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "Model_Var.h"
#include <cstddef>

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

void Model_Var::set_pos_in_con(const size_t term_idx,
                               const size_t pos_in_con)
{
  m_pos_in_con_list[term_idx] = pos_in_con;
}
