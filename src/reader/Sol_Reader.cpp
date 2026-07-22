/*=====================================================================================

    Filename:     Sol_Reader.cpp

    Description:  Solution file reader for warm-start values
        Version:  2.0

    Authors:      Alexander Hoen, HTW Berlin
                  Peng Lin, peng.lin.csor@gmail.com

    Note:         Warm-start solution loading was developed in collaboration
                  by Alexander Hoen and Peng Lin.

=====================================================================================*/

#include "Sol_Reader.h"
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

Sol_Read_Result Sol_Reader::read(const std::string& p_sol_file,
                                 const Model_Manager& p_model_manager,
                                 std::vector<double>& p_solution,
                                 std::vector<char>* p_loaded_mask)
{
  p_solution.assign(p_model_manager.var_num(), 0.0);
  if (p_loaded_mask != nullptr)
    p_loaded_mask->assign(p_model_manager.var_num(), 0);

  std::ifstream input(p_sol_file);
  if (!input.is_open())
  {
    return {false,
            0,
            0,
            "cannot open start solution file '" + p_sol_file + "'"};
  }

  const auto& var_name_to_idx = p_model_manager.var_name_to_idx();
  std::vector<char> local_seen_var(
      p_loaded_mask == nullptr ? p_model_manager.var_num() : 0, 0);
  std::vector<char>& seen_var =
      p_loaded_mask == nullptr ? local_seen_var : *p_loaded_mask;
  size_t loaded_var_num = 0;
  size_t unknown_var_num = 0;
  std::string line;
  size_t line_no = 0;

  while (std::getline(input, line))
  {
    ++line_no;
    const size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos)
      line = line.substr(0, comment_pos);

    line = trim(line);
    if (line.empty())
      continue;

    std::istringstream iss(line);
    std::string name;
    std::string value_text;
    iss >> name;
    iss >> value_text;

    if (name == "Variable" && value_text == "name")
      continue;

    auto var_iter = var_name_to_idx.find(name);
    if (var_iter == var_name_to_idx.end())
    {
      ++unknown_var_num;
      continue;
    }

    if (value_text.empty())
    {
      std::ostringstream oss;
      oss << "missing value for variable '" << name << "' in "
          << p_sol_file << ":" << line_no;
      return {false, loaded_var_num, unknown_var_num, oss.str()};
    }

    double value = 0.0;
    if (!parse_value(value_text, value))
    {
      std::ostringstream oss;
      oss << "invalid value '" << value_text << "' for variable '" << name
          << "' in " << p_sol_file << ":" << line_no;
      return {false, loaded_var_num, unknown_var_num, oss.str()};
    }

    std::string extra;
    if (iss >> extra)
    {
      std::ostringstream oss;
      oss << "unexpected token '" << extra << "' in " << p_sol_file << ":"
          << line_no;
      return {false, loaded_var_num, unknown_var_num, oss.str()};
    }

    size_t var_idx = var_iter->second;
    const auto& model_var = p_model_manager.var(var_idx);
    const double input_value = value;
    if (!model_var.try_normalize_value(value))
    {
      std::ostringstream oss;
      oss << "value '" << value_text << "' for variable '" << name << "' ";
      if (model_var.requires_integrality() &&
          !is_integral_within_tolerance(input_value))
        oss << "violates integrality";
      else if (!model_var.in_bound(input_value))
        oss << "is out of bounds [" << model_var.lower_bound() << ", "
            << model_var.upper_bound() << "]";
      else
        oss << "is outside its variable domain";
      oss << " in " << p_sol_file << ":" << line_no;
      return {false, loaded_var_num, unknown_var_num, oss.str()};
    }

    if (seen_var[var_idx])
    {
      std::ostringstream oss;
      oss << "duplicate value for variable '" << name << "' in "
          << p_sol_file << ":" << line_no;
      return {false, loaded_var_num, unknown_var_num, oss.str()};
    }

    p_solution[var_idx] = value;
    seen_var[var_idx] = 1;
    ++loaded_var_num;
  }

  return {true, loaded_var_num, unknown_var_num, ""};
}

std::string Sol_Reader::trim(const std::string& p_text)
{
  size_t first = 0;
  while (first < p_text.size() &&
         std::isspace(static_cast<unsigned char>(p_text[first])))
    ++first;

  if (first == p_text.size())
    return "";

  size_t last = p_text.size() - 1;
  while (last > first &&
         std::isspace(static_cast<unsigned char>(p_text[last])))
    --last;

  return p_text.substr(first, last - first + 1);
}

bool Sol_Reader::parse_value(const std::string& p_text, double& p_value)
{
  char* end = nullptr;
  errno = 0;
  p_value = std::strtod(p_text.c_str(), &end);

  if (end == p_text.c_str() || errno == ERANGE)
    return false;

  while (*end != '\0')
  {
    if (!std::isspace(static_cast<unsigned char>(*end)))
      return false;
    ++end;
  }

  return std::isfinite(p_value);
}
