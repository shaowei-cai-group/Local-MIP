/*=====================================================================================

    Filename:     solver_error.h

    Description:  Solver level exception type definition

=====================================================================================*/

#pragma once

#include <stdexcept>
#include <string>

class Solver_Error : public std::runtime_error
{
public:
  explicit Solver_Error(const std::string& p_message)
      : std::runtime_error(p_message)
  {
  }

  explicit Solver_Error(const char* p_message)
      : std::runtime_error(p_message)
  {
  }
};
