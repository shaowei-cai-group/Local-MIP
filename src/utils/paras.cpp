/*=====================================================================================

    Filename:     paras.cpp

    Description:  Parameter management and command line argument parsing
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "cmdline.h"
#include "global_defs.h"
#include "paras.h"
#include "string_utils.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

Paras g_paras;

namespace
{

bool only_missing_required_errors(const std::string& errors)
{
  if (errors.empty())
    return true;

  std::istringstream iss(errors);
  std::string line;
  bool has_line = false;
  while (std::getline(iss, line))
  {
    std::string trimmed = string_utils::trim_copy(line);
    if (trimmed.empty())
      continue;
    has_line = true;
    if (trimmed.rfind("need option: --", 0) != 0)
      return false;
  }
  return has_line;
}

[[noreturn]] void report_parameter_error(const std::string& message,
                                         bool p_exit_on_error)
{
  if (p_exit_on_error)
  {
    std::fprintf(stderr, "c [error] %s\n", message.c_str());
    std::exit(EXIT_FAILURE);
  }
  throw std::runtime_error(message);
}

template <typename T>
T parse_numeric_parameter(const std::string& name,
                          const std::string& value,
                          T p_low,
                          T p_high,
                          size_t p_line_no,
                          const std::string& p_file_path,
                          bool p_exit_on_error)
{
  static_assert(std::is_same_v<T, int> || std::is_same_v<T, double>);
  auto report_error = [&](const std::string& message) -> void
  {
    std::ostringstream oss;
    oss << message << " (file: " << p_file_path << ", line: "
        << p_line_no << ")";
    report_parameter_error(oss.str(), p_exit_on_error);
  };

  size_t parsed_chars = 0;
  if constexpr (std::is_same_v<T, int>)
  {
    long long parsed_value = 0;
    try
    {
      parsed_value = std::stoll(value, &parsed_chars);
      if (parsed_chars != value.size())
        throw std::invalid_argument("trailing characters");
    }
    catch (const std::exception&)
    {
      report_error("invalid integer value '" + value +
                   "' for parameter '" + name + "'");
    }
    if (parsed_value < p_low || parsed_value > p_high)
    {
      std::ostringstream oss;
      oss << "value '" << value << "' for parameter '" << name
          << "' is out of range [" << p_low << ", " << p_high << "]";
      report_error(oss.str());
    }
    return static_cast<int>(parsed_value);
  }
  else
  {
    double parsed_value = 0.0;
    try
    {
      parsed_value = std::stod(value, &parsed_chars);
      if (parsed_chars != value.size())
        throw std::invalid_argument("trailing characters");
    }
    catch (const std::exception&)
    {
      report_error("invalid floating value '" + value +
                   "' for parameter '" + name + "'");
    }
    if (!std::isfinite(parsed_value) || parsed_value < p_low ||
        parsed_value > p_high)
    {
      std::ostringstream oss;
      oss << "value '" << value << "' for parameter '" << name
          << "' is out of range [" << p_low << ", " << p_high << "]";
      report_error(oss.str());
    }
    return parsed_value;
  }
}

template <typename T>
void print_numeric_parameter(const char* p_name,
                             const char* p_type,
                             T p_value,
                             T p_default,
                             const char* p_comment)
{
  static_assert(std::is_same_v<T, int> || std::is_same_v<T, double>);
  if constexpr (std::is_same_v<T, int>)
  {
    printf("c %-20s\t %-10s\t %-10d\t %-10d\t %s\n",
           p_name, p_type, p_value, p_default, p_comment);
  }
  else
  {
    printf("c %-20s\t %-10s\t %-10f\t %-10f\t %s\n",
           p_name, p_type, p_value, p_default, p_comment);
  }
}

} // namespace

void Paras::parse_args(int argc, char* argv[])
{
  cmdline::parser parser;
  parser.add("help", 'h', "print this message");

#define STR_PARA(N, S, M, D, C) parser.add<std::string>(#N, S, C, M, D);
  STR_PARAS
#undef STR_PARA

#define PARA(N, T, S, M, D, L, H, C)                                      \
  parser.add<T>(#N,                                                       \
                S,                                                        \
                C,                                                        \
                M,                                                        \
                T{D},                                                      \
                cmdline::range<T>(T{L}, T{H}));
  PARAS
#undef PARA

  std::vector<std::string> normalized_args;
  normalized_args.reserve(static_cast<size_t>(argc));
  for (int arg_idx = 0; arg_idx < argc; ++arg_idx)
    normalized_args.emplace_back(argv[arg_idx]);

  for (size_t arg_idx = 1; arg_idx < normalized_args.size(); ++arg_idx)
  {
    if (normalized_args[arg_idx] == "-?")
    {
      normalized_args[arg_idx] = "--help";
      continue;
    }

    if (normalized_args[arg_idx] != "-h")
      continue;

    const bool has_value =
        arg_idx + 1 < normalized_args.size() &&
        !normalized_args[arg_idx + 1].empty() &&
        normalized_args[arg_idx + 1][0] != '-';
    if (has_value)
      normalized_args[arg_idx] = "-H";
  }

  std::vector<char*> normalized_argv;
  normalized_argv.reserve(normalized_args.size());
  for (std::string& arg : normalized_args)
    normalized_argv.push_back(arg.data());

  bool parsed_ok = parser.parse(argc, normalized_argv.data());
  if (!parsed_ok)
  {
    std::string errors = parser.error_full();
    if (!only_missing_required_errors(errors))
    {
      fprintf(stderr, "%s", errors.c_str());
      fprintf(stderr, "%s", parser.usage().c_str());
      exit(EXIT_FAILURE);
    }
  }

  if (parser.exist("help"))
  {
    fprintf(stdout, "%s", parser.usage().c_str());
    exit(0);
  }

  std::string config_path;
  if (parser.exist("param_set_file"))
    config_path = parser.get<std::string>("param_set_file");

  if (!config_path.empty())
    load_from_file(config_path);

#define STR_PARA(N, S, M, D, C)                                           \
  if (parser.exist(#N))                                                   \
    this->N = parser.get<std::string>(#N);
  STR_PARAS
#undef STR_PARA

#define PARA(N, T, S, M, D, L, H, C)                                      \
  if (parser.exist(#N))                                                   \
    this->N = parser.get<T>(#N);
  PARAS
#undef PARA

  if (!config_path.empty() && param_set_file.empty())
    param_set_file = config_path;

  validate_required();

  if (!std::isfinite(time_limit) || time_limit <= 0.0)
    report_parameter_error("time_limit must be finite and positive", true);
  if (!std::isfinite(feas_tolerance) || feas_tolerance < 0.0)
  {
    report_parameter_error(
        "feas_tolerance must be finite and non-negative", true);
  }
  if (!std::isfinite(opt_tolerance) || opt_tolerance < 0.0)
  {
    report_parameter_error(
        "opt_tolerance must be finite and non-negative", true);
  }
  if (!std::isfinite(zero_tolerance) || zero_tolerance < 0.0)
  {
    report_parameter_error(
        "zero_tolerance must be finite and non-negative", true);
  }

}

void Paras::load_from_file(const std::string& file_path, bool p_exit_on_error)
{
  m_loaded_param_names.clear();

  std::ifstream input(file_path);
  if (!input.is_open())
  {
    report_parameter_error("cannot open parameter set file '" + file_path +
                               "'",
                           p_exit_on_error);
  }
  printf("c parameter set file is set to : %s\n", file_path.c_str());

  std::string line;
  size_t line_no = 0;
  while (std::getline(input, line))
  {
    ++line_no;

    size_t comment_pos = line.find_first_of("#;");
    if (comment_pos != std::string::npos)
      line = line.substr(0, comment_pos);

    std::string trimmed_line = string_utils::trim_copy(line);
    if (trimmed_line.empty())
      continue;
    if (trimmed_line[0] == 'c' &&
        (trimmed_line.size() == 1 ||
         std::isspace(static_cast<unsigned char>(trimmed_line[1]))))
      continue;

    size_t equal_pos = trimmed_line.find('=');
    std::string name;
    std::string value;
    if (equal_pos != std::string::npos)
    {
      name = string_utils::trim_copy(trimmed_line.substr(0, equal_pos));
      value = string_utils::trim_copy(trimmed_line.substr(equal_pos + 1));
    }
    else
    {
      std::istringstream iss(trimmed_line);
      if (!(iss >> name))
        continue;
      std::string rest;
      std::getline(iss >> std::ws, rest);
      value = string_utils::trim_copy(rest);
    }

    if (name.empty() || value.empty())
    {
      std::ostringstream oss;
      oss << "invalid parameter format in " << file_path << ":" << line_no;
      report_parameter_error(oss.str(), p_exit_on_error);
    }

    if (!set_param_from_string(
            name, value, line_no, file_path, p_exit_on_error))
    {
      std::ostringstream oss;
      oss << "unknown parameter '" << name << "' in " << file_path << ":"
          << line_no;
      report_parameter_error(oss.str(), p_exit_on_error);
    }
    m_loaded_param_names.insert(name);
  }
}

bool Paras::has_loaded_param(const std::string& name) const
{
  return m_loaded_param_names.contains(name);
}

bool Paras::set_param_from_string(const std::string& name,
                                  const std::string& value,
                                  size_t line_no,
                                  const std::string& file_path,
                                  bool p_exit_on_error)
{
#define PARA(N, T, S, M, D, L, H, C)                                      \
  if (name == #N)                                                         \
  {                                                                       \
    this->N = parse_numeric_parameter<T>(                                 \
        name, value, T{L}, T{H},                                          \
        line_no, file_path, p_exit_on_error);                             \
    return true;                                                          \
  }
  PARAS
#undef PARA

#define STR_PARA(N, S, M, D, C)                                           \
  if (name == #N)                                                         \
  {                                                                       \
    this->N = value;                                                      \
    return true;                                                          \
  }
  STR_PARAS
#undef STR_PARA

  return false;
}

void Paras::validate_required() const
{
#define STR_PARA(N, S, M, D, C)                                           \
  if (M && this->N.empty())                                               \
  {                                                                       \
    fprintf(                                                              \
        stderr, "c [error] required parameter '%s' is missing.\n", #N);   \
    exit(EXIT_FAILURE);                                                   \
  }
  STR_PARAS
#undef STR_PARA
}

void Paras::print_change()
{
  printf("c ------------------- Paras list -------------------\n");
  printf("c %-20s\t %-10s\t %-10s\t %-10s\t %s\n",
         "Name",
         "Type",
         "Now",
         "Default",
         "Comment");

#define PARA(N, T, S, M, D, L, H, C)                                      \
  print_numeric_parameter<T>((#N), (#T), N, T{D}, (C));
  PARAS
#undef PARA

#define STR_PARA(N, S, M, D, C)                                           \
  printf("c %-20s\t string\t\t %-10s\t %-10s\t %s\n",                     \
         (#N),                                                            \
         N.c_str(),                                                       \
         (#D),                                                            \
         (C));
  STR_PARAS
#undef STR_PARA

  printf("c --------------------------------------------------\n");
}
