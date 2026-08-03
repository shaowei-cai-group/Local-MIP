/*=====================================================================================

    Filename:     Model_Reader.cpp

    Description:  Unified model file reading
        Version:  2.0

=====================================================================================*/

#include "../utils/solver_error.h"
#include "LP_Reader.h"
#include "MPS_Reader.h"
#include "Model_Reader.h"
#include <cctype>
#include <string>

void read_model_file(const std::string& p_model_file,
                     Model_Manager& p_model_manager)
{
  if (p_model_file.empty())
    throw Solver_Error("model file path is empty");

  const auto dot_pos = p_model_file.find_last_of('.');
  std::string extension =
      dot_pos == std::string::npos ? "" : p_model_file.substr(dot_pos + 1);
  for (char& ch : extension)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

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
