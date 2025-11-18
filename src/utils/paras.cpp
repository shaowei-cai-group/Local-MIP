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
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>

Paras g_paras;

namespace
{

std::string trim(const std::string& text)
{
  size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(text[first])))
    ++first;
  if (first == text.size())
    return "";
  size_t last = text.size() - 1;
  while (last > first &&
         std::isspace(static_cast<unsigned char>(text[last])))
    --last;
  return text.substr(first, last - first + 1);
}

bool only_missing_required_errors(const std::string& errors)
{
  if (errors.empty())
    return true;

  std::istringstream iss(errors);
  std::string line;
  bool has_line = false;
  while (std::getline(iss, line))
  {
    std::string trimmed = trim(line);
    if (trimmed.empty())
      continue;
    has_line = true;
    if (trimmed.rfind("need option: --", 0) != 0)
      return false;
  }
  return has_line;
}

} // namespace

void Paras::parse_args(int argc, char* argv[])
{
  cmdline::parser parser;
  parser.add("help", '?', "print this message");

#define STR_PARA(N, S, M, D, C) parser.add<std::string>(#N, S, C, M, D);
  STR_PARAS
#undef STR_PARA

#define PARA(N, T, S, M, D, L, H, C)                                      \
  if (!strcmp(#T, "int"))                                                 \
    parser.add<int>(#N, S, C, M, D, cmdline::range((int)L, (int)H));      \
  else                                                                    \
    parser.add<double>(                                                   \
        #N, S, C, M, D, cmdline::range((double)L, (double)H));
  PARAS
#undef PARA

  bool parsed_ok = parser.parse(argc, argv);
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
  {                                                                       \
    if (!strcmp(#T, "int"))                                               \
      this->N = parser.get<int>(#N);                                      \
    else                                                                  \
      this->N = parser.get<double>(#N);                                   \
  }
  PARAS
#undef PARA

  if (!config_path.empty() && param_set_file.empty())
    param_set_file = config_path;

  validate_required();

  k_feas_tolerance = feas_tolerance;
  k_opt_tolerance = opt_tolerance;
  k_zero_tolerance = zero_tolerance;
}

void Paras::load_from_file(const std::string& file_path)
{
  std::ifstream input(file_path);
  if (!input.is_open())
  {
    fprintf(stderr,
            "c [error] cannot open parameter set file '%s'\n",
            file_path.c_str());
    exit(EXIT_FAILURE);
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

    std::string trimmed_line = trim(line);
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
      name = trim(trimmed_line.substr(0, equal_pos));
      value = trim(trimmed_line.substr(equal_pos + 1));
    }
    else
    {
      std::istringstream iss(trimmed_line);
      if (!(iss >> name))
        continue;
      std::string rest;
      std::getline(iss >> std::ws, rest);
      value = trim(rest);
    }

    if (name.empty() || value.empty())
    {
      fprintf(stderr,
              "c [error] invalid parameter format in %s:%zu\n",
              file_path.c_str(),
              line_no);
      exit(EXIT_FAILURE);
    }

    if (!set_param_from_string(name, value, line_no, file_path))
    {
      fprintf(stderr,
              "c [error] unknown parameter '%s' in %s:%zu\n",
              name.c_str(),
              file_path.c_str(),
              line_no);
      exit(EXIT_FAILURE);
    }
  }
}

bool Paras::set_param_from_string(const std::string& name,
                                  const std::string& value,
                                  size_t line_no,
                                  const std::string& file_path)
{
  auto report_error = [&](const std::string& message)
  {
    fprintf(stderr,
            "c [error] %s (file: %s, line: %zu)\n",
            message.c_str(),
            file_path.c_str(),
            line_no);
    exit(EXIT_FAILURE);
  };

#define PARA(N, T, S, M, D, L, H, C)                                      \
  if (name == #N)                                                         \
  {                                                                       \
    if (!strcmp(#T, "int"))                                               \
    {                                                                     \
      long long parsed_value = 0;                                         \
      try                                                                 \
      {                                                                   \
        parsed_value = std::stoll(value);                                 \
      }                                                                   \
      catch (const std::exception&)                                       \
      {                                                                   \
        report_error("invalid integer value '" + value +                  \
                     "' for parameter '" + name + "'");                   \
      }                                                                   \
      if (parsed_value < static_cast<long long>(L) ||                     \
          parsed_value > static_cast<long long>(H))                       \
      {                                                                   \
        std::ostringstream oss;                                           \
        oss << "value '" << value << "' for parameter '" << name          \
            << "' is out of range [" << static_cast<long long>(L) << ", " \
            << static_cast<long long>(H) << "]";                          \
        report_error(oss.str());                                          \
      }                                                                   \
      this->N = static_cast<int>(parsed_value);                           \
    }                                                                     \
    else                                                                  \
    {                                                                     \
      double parsed_value = 0.0;                                          \
      try                                                                 \
      {                                                                   \
        parsed_value = std::stod(value);                                  \
      }                                                                   \
      catch (const std::exception&)                                       \
      {                                                                   \
        report_error("invalid floating value '" + value +                 \
                     "' for parameter '" + name + "'");                   \
      }                                                                   \
      if (parsed_value < static_cast<double>(L) ||                        \
          parsed_value > static_cast<double>(H))                          \
      {                                                                   \
        std::ostringstream oss;                                           \
        oss << "value '" << value << "' for parameter '" << name          \
            << "' is out of range [" << static_cast<double>(L) << ", "    \
            << static_cast<double>(H) << "]";                             \
        report_error(oss.str());                                          \
      }                                                                   \
      this->N = parsed_value;                                             \
    }                                                                     \
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
  if (!strcmp(#T, "int"))                                                 \
    printf("c %-20s\t %-10s\t %-10d\t %-10d\t %s\n",                      \
           (#N),                                                          \
           (#T),                                                          \
           (int)N,                                                        \
           (int)(D),                                                      \
           (C));                                                          \
  else                                                                    \
    printf("c %-20s\t %-10s\t %-10f\t %-10f\t %s\n",                      \
           (#N),                                                          \
           (#T),                                                          \
           (double)N,                                                     \
           (double)(D),                                                   \
           (C));
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
