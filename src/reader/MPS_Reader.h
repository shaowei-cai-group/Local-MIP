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
#include <cstddef>
#include <sstream>
#include <string>

class MPS_Reader : public Model_Reader
{
private:
  Model_Manager* m_model_manager;

  std::istringstream m_iss;

  std::string m_read_line;

  bool m_integrality_marker;

  inline void iss_setup();

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
  m_iss.str(m_read_line);
  m_iss.seekg(0, std::ios::beg);
}


inline bool MPS_Reader::is_blank(const std::string& a) const
{
  for (const char *p = a.data(), *end = p + a.size(); p != end; ++p)
    if (!std::isspace(static_cast<unsigned char>(*p)))
      return false;
  return true;
}

inline void MPS_Reader::printf_error_line(const std::string& a) const
{
  std::string message = "c error line: " + a;
  printf("%s\n", message.c_str());
  throw Solver_Error(message);
}