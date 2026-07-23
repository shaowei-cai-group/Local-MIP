/*=====================================================================================

    Filename:     MPS_Reader.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../model_data/Model_Manager.h"
#include "../utils/solver_error.h"
#include "Model_Reader.h"
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_set>

class MPS_Reader : public Model_Reader
{
private:
  Model_Manager* m_model_manager;

  std::istringstream m_iss;

  std::string m_read_line;

  bool m_integrality_marker;

  size_t m_small_coeff_counter;

  std::unordered_set<std::string> m_ignored_free_rows;

  inline size_t record_data_size(const std::string& p_record) const;

  inline void iss_setup();

  bool read_optional_bound_value(double& p_value);

  void add_coeff_var_to_con(const std::string& p_con_name,
                            double p_coeff,
                            const std::string& p_var_name);

  void print_con(const Model_Con& p_con);

  inline bool is_blank(const std::string& p) const;

  inline void printf_error_line(const std::string& p) const;

public:
  MPS_Reader(Model_Manager* p_model_manager);

  ~MPS_Reader() override = default;

  void read(const char* p_model_file) override;
};

inline void MPS_Reader::iss_setup()
{
  m_iss.clear();
  const size_t data_size = record_data_size(m_read_line);
  if (data_size == m_read_line.size())
    m_iss.str(m_read_line);
  else
    m_iss.str(m_read_line.substr(0, data_size));
  m_iss.seekg(0, std::ios::beg);
}

inline size_t
MPS_Reader::record_data_size(const std::string& p_record) const
{
  for (size_t idx = 1; idx < p_record.size(); ++idx)
  {
    if (p_record[idx] == '$' &&
        std::isspace(static_cast<unsigned char>(p_record[idx - 1])))
      return idx;
  }
  return p_record.size();
}

inline bool MPS_Reader::is_blank(const std::string& p_record) const
{
  const size_t data_size = record_data_size(p_record);
  for (size_t idx = 0; idx < data_size; ++idx)
    if (!std::isspace(static_cast<unsigned char>(p_record[idx])))
      return false;
  return true;
}

inline void MPS_Reader::printf_error_line(const std::string& a) const
{
  std::string message = "c error line: " + a;
  printf("%s\n", message.c_str());
  throw Solver_Error(message);
}
