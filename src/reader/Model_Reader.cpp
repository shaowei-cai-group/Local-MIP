/*=====================================================================================

    Filename:     Model_Reader.cpp

    Description:  Unified model file reading
        Version:  2.0

=====================================================================================*/

#include "../utils/solver_error.h"
#include "../utils/string_utils.h"
#include "LP_Reader.h"
#include "MPS_Reader.h"
#include "Model_Reader.h"
#include <string>

void read_model_file(const std::string& p_model_file,
                     Model_Manager& p_model_manager)
{
  if (p_model_file.empty())
    throw Solver_Error("model file path is empty");

  const auto dot_pos = p_model_file.find_last_of('.');
  const std::string extension =
      dot_pos == std::string::npos
          ? ""
          : string_utils::to_lower_copy(p_model_file.substr(dot_pos + 1));

  if (extension == "mps")
  {
    MPS_Reader reader(&p_model_manager);
    reader.read(p_model_file.c_str());
    return;
  }
  if (extension == "lp")
  {
    LP_Reader reader(&p_model_manager);
    reader.read(p_model_file.c_str());
    return;
  }
  throw Solver_Error("unsupported model file format: " + p_model_file);
}
