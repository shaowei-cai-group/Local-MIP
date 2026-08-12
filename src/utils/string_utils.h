#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>

namespace string_utils
{

inline std::string to_lower_copy(std::string p_value)
{
  std::transform(p_value.begin(),
                 p_value.end(),
                 p_value.begin(),
                 [](unsigned char p_char)
                 { return static_cast<char>(std::tolower(p_char)); });
  return p_value;
}

inline std::string to_upper_copy(std::string p_value)
{
  std::transform(p_value.begin(),
                 p_value.end(),
                 p_value.begin(),
                 [](unsigned char p_char)
                 { return static_cast<char>(std::toupper(p_char)); });
  return p_value;
}

inline std::string trim_copy(const std::string& p_value)
{
  size_t first = 0;
  while (first < p_value.size() &&
         std::isspace(static_cast<unsigned char>(p_value[first])))
    ++first;
  if (first == p_value.size())
    return "";

  size_t last = p_value.size() - 1;
  while (last > first &&
         std::isspace(static_cast<unsigned char>(p_value[last])))
    --last;
  return p_value.substr(first, last - first + 1);
}

} // namespace string_utils
