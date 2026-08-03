/*=====================================================================================

    Filename:     Model_Var.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../utils/global_defs.h"
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

class Model_Var
{
private:
  friend class Model_Manager;

  std::string m_name;

  size_t m_idx;

  double m_upper_bound;

  double m_lower_bound;

  std::vector<size_t> m_con_idx_list;

  std::vector<size_t> m_pos_in_con_list;

  Var_Type m_type;

  bool m_requires_integrality;

public:
  Model_Var(const std::string& p_name,
            size_t p_idx,
            bool p_requires_integrality);

  ~Model_Var();

  inline void add_con(const size_t p_con_idx, const size_t p_pos_in_con);

  void set_pos_in_con(const size_t p_term_idx, const size_t p_pos_in_con);

  inline bool is_real() const;

  inline bool is_general_integer() const;

  inline bool requires_integrality() const;

  inline size_t term_num() const;

  inline double upper_bound() const;

  inline double lower_bound() const;

  inline size_t pos_in_con(const size_t p_term_idx) const;

  inline size_t con_idx(const size_t p_term_idx) const;

  inline const std::vector<size_t>& con_idx_set() const;

  inline Var_Type type() const;

  inline size_t idx() const;

  inline const std::string& name() const;
};

inline bool Model_Var::is_real() const
{
  return m_type == Var_Type::real;
}

inline bool Model_Var::is_general_integer() const
{
  return m_type == Var_Type::general_integer;
}

inline bool Model_Var::requires_integrality() const
{
  return m_requires_integrality;
}

inline size_t Model_Var::term_num() const
{
  return m_con_idx_list.size();
}

inline double Model_Var::upper_bound() const
{
  return m_upper_bound;
}

inline double Model_Var::lower_bound() const
{
  return m_lower_bound;
}

inline size_t Model_Var::pos_in_con(const size_t p_term_idx) const
{
  return m_pos_in_con_list[p_term_idx];
}

inline size_t Model_Var::con_idx(const size_t p_term_idx) const
{
  return m_con_idx_list[p_term_idx];
}

inline const std::vector<size_t>& Model_Var::con_idx_set() const
{
  return m_con_idx_list;
}

inline Var_Type Model_Var::type() const
{
  return m_type;
}

inline size_t Model_Var::idx() const
{
  return m_idx;
}

inline const std::string& Model_Var::name() const
{
  return m_name;
}

inline void Model_Var::add_con(const size_t con_idx,
                               const size_t pos_in_con)
{
  m_con_idx_list.push_back(con_idx);
  m_pos_in_con_list.push_back(pos_in_con);
}
