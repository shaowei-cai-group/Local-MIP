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

#include "local_mip/Local_MIP.h"
#include <cstdio>

int main()
{
  Local_MIP solver;
  solver.set_model_file("test-set/2club200v15p5scn.mps");
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
