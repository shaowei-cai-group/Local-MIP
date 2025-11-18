/*=====================================================================================

    Filename:     Model_Manager.h

    Description:  Model Manager
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../utils/global_defs.h"
#include "Model_Con.h"
#include "Model_Var.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Model_Manager
{
private:
  int m_bound_strengthen;

  std::unordered_map<std::string, size_t> m_var_name_to_idx;

  std::unordered_map<std::string, size_t> m_con_name_to_idx;

  std::vector<size_t> m_var_idx_to_obj_idx;

  std::string m_obj_name;

  std::vector<Model_Con> m_con_list;

  std::vector<Model_Var> m_var_list;

  std::vector<size_t> m_binary_idx_list;

  std::vector<bool> m_con_is_equality;

  std::vector<size_t> m_non_fixed_var_idxs;

  std::vector<double> m_var_obj_cost;

  int m_is_min;

  double m_obj_offset;

  size_t m_var_num;

  size_t m_general_integer_num;

  size_t m_binary_num;

  size_t m_fixed_num;

  size_t m_real_num;

  size_t m_con_num;

  std::unordered_map<Con_Type, std::vector<size_t>> m_type_to_con_idx_list;

  std::unordered_map<Con_Type, std::unordered_set<size_t>>
      m_type_to_con_idx_set;

  size_t m_delete_con_num;

  size_t m_delete_var_num;

  size_t m_infer_var_num;

  bool m_split_eq;

public:
  Model_Manager();

  ~Model_Manager();

  size_t make_var(const std::string& p_name, const bool p_integrality);

  size_t make_con(const std::string& p_name, const char p_type = '<');

  inline void set_rhs(const std::string& p_name, const double p_rhs);

  inline void setup_max();

  inline void add_obj_offset(const double p_offset);

  inline void set_obj_name(const std::string& p_obj_name);

  inline Model_Var& var(const size_t p_idx);

  inline Model_Var& var(const std::string& p_name);

  inline Model_Con& con(const size_t p_idx);

  inline Model_Con& con(const std::string& p_name);

  inline void set_bound_strengthen(const int p_enable);

  inline void set_split_eq(bool p_enable);

  bool process_after_read();

  inline const std::string& get_obj_name() const;

  inline const Model_Var& var(const size_t p_idx) const;

  inline const Model_Con& con(const size_t p_idx) const;

  inline const Model_Con& obj() const;

  inline size_t var_num() const;

  inline size_t con_num() const;

  inline size_t general_integer_num() const;

  inline size_t binary_num() const;

  inline size_t fixed_num() const;

  inline size_t real_num() const;

  inline int is_min() const;

  inline double obj_offset() const;

  inline size_t var_id_to_obj_idx(const size_t p_var_idx) const;

  inline bool exists_var(const std::string& p_name) const;

  inline size_t con_idx(const std::string& p_name) const;

  inline const std::vector<size_t>& binary_idx_list() const;

  inline const std::vector<bool>& con_is_equality() const;

  inline const std::vector<size_t>& non_fixed_var_idxs() const;

  inline const std::vector<double>& var_obj_cost() const;

private:
  bool tighten_bounds();

  bool global_propagation();

  bool calculate_vars();

  bool singleton_deduction(Model_Con& p_con);

  void classify_con(Model_Con& p_con);

  void print_cons_type_summary() const;

  void convert_eq_to_ineq();

  void append_negated_con(const Model_Con& p_source);

  std::string
  make_duplicate_constraint_name(const std::string& p_base) const;
};

inline const std::string& Model_Manager::get_obj_name() const
{
  return m_obj_name;
}

inline size_t Model_Manager::var_num() const
{
  return m_var_num;
}

inline size_t Model_Manager::con_num() const
{
  return m_con_num;
}

inline size_t Model_Manager::general_integer_num() const
{
  return m_general_integer_num;
}

inline size_t Model_Manager::binary_num() const
{
  return m_binary_num;
}

inline size_t Model_Manager::fixed_num() const
{
  return m_fixed_num;
}

inline size_t Model_Manager::real_num() const
{
  return m_real_num;
}

inline int Model_Manager::is_min() const
{
  return m_is_min;
}

inline double Model_Manager::obj_offset() const
{
  return m_obj_offset;
}

inline size_t Model_Manager::var_id_to_obj_idx(size_t p_var_idx) const
{
  assert(p_var_idx < m_var_idx_to_obj_idx.size());
  return m_var_idx_to_obj_idx[p_var_idx];
}

inline bool Model_Manager::exists_var(const std::string& p_name) const
{
  return m_var_name_to_idx.find(p_name) != m_var_name_to_idx.end();
}

inline size_t Model_Manager::con_idx(const std::string& p_name) const
{
  auto iter = m_con_name_to_idx.find(p_name);
  assert(iter != m_con_name_to_idx.end());
  return iter->second;
}

inline const Model_Var& Model_Manager::var(const size_t p_idx) const
{
  assert(p_idx < m_var_list.size());
  return m_var_list[p_idx];
}

inline Model_Var& Model_Manager::var(const size_t p_idx)
{
  assert(p_idx < m_var_list.size());
  return m_var_list[p_idx];
}

inline Model_Var& Model_Manager::var(const std::string& p_name)
{
  auto iter = m_var_name_to_idx.find(p_name);
  assert(iter != m_var_name_to_idx.end());
  return m_var_list[iter->second];
}

inline Model_Con& Model_Manager::con(const size_t p_idx)
{
  assert(p_idx < m_con_list.size());
  return m_con_list[p_idx];
}

inline Model_Con& Model_Manager::con(const std::string& p_name)
{
  auto iter = m_con_name_to_idx.find(p_name);
  assert(iter != m_con_name_to_idx.end());
  return m_con_list[iter->second];
}

inline const Model_Con& Model_Manager::con(const size_t p_idx) const
{
  assert(p_idx < m_con_list.size());
  return m_con_list[p_idx];
}

inline const Model_Con& Model_Manager::obj() const
{
  assert(!m_con_list.empty());
  return m_con_list[0];
}

inline void Model_Manager::add_obj_offset(const double p_offset)
{
  m_obj_offset += p_offset;
}

inline void Model_Manager::setup_max()
{
  m_is_min = -1;
}

inline void Model_Manager::set_rhs(const std::string& p_name,
                                   const double p_rhs)
{
  m_con_list[m_con_name_to_idx[p_name]].set_rhs(p_rhs);
}

inline void Model_Manager::set_obj_name(const std::string& p_obj_name)
{
  m_obj_name = p_obj_name;
}

void Model_Manager::set_bound_strengthen(const int p_level)
{
  m_bound_strengthen = p_level;
}

inline void Model_Manager::set_split_eq(bool p_enable)
{
  m_split_eq = p_enable;
}

inline const std::vector<size_t>& Model_Manager::binary_idx_list() const
{
  return m_binary_idx_list;
}

inline const std::vector<bool>& Model_Manager::con_is_equality() const
{
  return m_con_is_equality;
}

inline const std::vector<size_t>& Model_Manager::non_fixed_var_idxs() const
{
  return m_non_fixed_var_idxs;
}

inline const std::vector<double>& Model_Manager::var_obj_cost() const
{
  return m_var_obj_cost;
}
