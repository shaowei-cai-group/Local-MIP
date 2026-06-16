/*=====================================================================================

    Filename:     Sol_Reader.h

    Description:  Solution file reader for warm-start values
        Version:  2.0

    Authors:      Alexander Hoen, HTW Berlin
                  Peng Lin, peng.lin.csor@gmail.com

    Note:         Warm-start solution loading was developed in collaboration
                  by Alexander Hoen and Peng Lin.

=====================================================================================*/

#pragma once

#include "../model_data/Model_Manager.h"
#include <cstddef>
#include <string>
#include <vector>

struct Sol_Read_Result
{
  bool m_success;

  size_t m_loaded_var_num;

  size_t m_unknown_var_num;

  std::string m_message;
};

class Sol_Reader
{
public:
  static Sol_Read_Result read(const std::string& p_sol_file,
                              const Model_Manager& p_model_manager,
                              std::vector<double>& p_solution);

private:
  static std::string trim(const std::string& p_text);

  static bool parse_value(const std::string& p_text, double& p_value);
};
