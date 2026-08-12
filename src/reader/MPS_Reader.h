/*=====================================================================================

    Filename:     MPS_Reader.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../utils/solver_error.h"
#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_set>

class Model_Manager;

class MPS_Reader
{
private:
  Model_Manager* m_model_manager;

  std::istringstream m_iss;

  std::string m_read_line;

  bool m_integrality_marker;

  size_t m_small_coeff_counter;

  std::unordered_set<std::string> m_ignored_free_rows;

  size_t record_data_size(const std::string& p_record) const;

  void iss_setup();

  bool read_optional_bound_value(double& p_value);

  void add_coeff_var_to_con(const std::string& p_con_name,
                            double p_coeff,
                            const std::string& p_var_name);

  bool is_blank(const std::string& p_record) const;

  [[noreturn]] void printf_error_line(const std::string& p_line) const;

public:
  MPS_Reader(Model_Manager* p_model_manager);

  void read(const char* p_model_file);
};
