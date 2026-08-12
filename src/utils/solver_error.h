/*=====================================================================================

    Filename:     solver_error.h

    Description:  Solver level exception type definition

=====================================================================================*/

#pragma once

#include <stdexcept>

class Solver_Error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};
