/*=====================================================================================

    Filename:     LP_Reader.h

    Description:  LP format file parser interface
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../model_data/Model_Manager.h"
#include "Model_Reader.h"
#include <cstddef>
#include <string>

namespace lp_internal
{
class Tokenizer;
}

class LP_Reader : public Model_Reader
{
public:
  explicit LP_Reader(Model_Manager* p_model_manager);

  ~LP_Reader() override = default;

  void read(const char* p_file_name) override;

private:
  Model_Manager* m_model_manager;

  size_t m_auto_con_counter;

  void parse_objective(lp_internal::Tokenizer& tokenizer);

  void parse_constraints(lp_internal::Tokenizer& tokenizer);

  void parse_bounds(lp_internal::Tokenizer& tokenizer);

  void parse_integers(lp_internal::Tokenizer& tokenizer);

  void parse_binaries(lp_internal::Tokenizer& tokenizer);

  void add_term(const std::string& con_name,
                const std::string& var_name,
                double coeff);

  std::string generate_constraint_name();
};
