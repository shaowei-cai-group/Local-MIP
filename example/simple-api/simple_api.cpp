/*=====================================================================================

    Filename:     simple_api.cpp

    Description:  Simple API Example - Demonstrates basic usage of Local-MIP
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "example_paths.h"
#include "local_mip/Local_MIP.h"
#include <cstdio>

namespace
{

constexpr const char kDefaultModelFile[] = "test-set/2club200v15p5scn.mps";

} // namespace

int main(int argc, char** argv)
{
  const std::string model_file = example_paths::resolve_demo_model_path_or_exit(
      argc, argv, kDefaultModelFile);

  Local_MIP solver;
  solver.set_model_file(model_file);
  solver.set_sol_path("example_simple.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);
  solver.run();

  // Get solution results
  if (solver.is_feasible())
  {
    printf("Solution is feasible!\n");
    printf("Objective value: %.10f\n", solver.get_obj_value());
    printf("Solution written to: example_simple.sol\n");
  }
  else
  {
    printf("No feasible solution found.\n");
  }

  return 0;
}
