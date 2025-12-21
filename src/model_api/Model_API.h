/*=====================================================================================

    Filename:     Model_API.h

    Description:  Modeling API for Local-MIP
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../utils/global_defs.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Model_Manager;

class Model_API
{
public:
  enum class Sense
  {
    minimize,
    maximize
  };

  Model_API();

  ~Model_API();

  void set_sense(Sense p_sense);

  bool set_obj_offset(double p_offset);

  int add_var(const std::string& p_name,
              double p_lb,
              double p_ub,
              double p_cost = 0.0,
              Var_Type p_type = Var_Type::real);

  bool set_cost(int p_col, double p_cost);

  bool set_cost(const std::string& p_name, double p_cost);

  int add_con(double p_lb,
              double p_ub,
              const std::vector<int>& p_cols,
              const std::vector<double>& p_coefs);

  int add_con(double p_lb,
              double p_ub,
              const std::vector<std::string>& p_names,
              const std::vector<double>& p_coefs);

  bool add_var_to_con(int p_row, int p_col, double p_coef);

  bool add_var_to_con(int p_row, const std::string& p_name, double p_coef);

  bool set_integrality(int p_col, Var_Type p_type);

  bool set_integrality(const std::string& p_name, Var_Type p_type);

  void build_model(Model_Manager& p_model_manager);

  inline size_t get_num_vars() const
  {
    return m_var_num;
  }

  inline size_t get_num_cons() const
  {
    return m_con_num;
  }

private:
  struct VarData
  {
    std::string m_name;
    double m_lb;
    double m_ub;
    double m_cost;
    Var_Type m_type;
  };

  struct ConData
  {
    double m_lb;
    double m_ub;
    std::vector<int> m_var_indices;
    std::vector<double> m_coefs;
  };

  Sense m_sense;

  double m_obj_offset;

  std::vector<VarData> m_vars;

  std::vector<ConData> m_cons;

  std::unordered_map<std::string, int> m_var_name_to_idx;

  size_t m_var_num;

  size_t m_con_num;

  int get_var_idx(const std::string& p_name) const;

  bool is_valid_var_idx(int p_idx) const;

  bool is_valid_con_idx(int p_idx) const;

  void add_vars_to_constraint(const ConData& con,
                              size_t con_idx,
                              Model_Manager& p_model_manager,
                              const std::vector<size_t>& api_to_mgr_idx);
};
