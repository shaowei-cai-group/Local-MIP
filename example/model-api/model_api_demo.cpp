/*=====================================================================================

    Filename:     model_api_demo.cpp

    Description:  Demonstration of Local-MIP Model API for programmatic model building
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include "model_api/Model_API.h"
#include "utils/global_defs.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

int main()
{
  const double inf = std::numeric_limits<double>::infinity();

  // Create a Local-MIP solver instance
  Local_MIP solver;

  // Enable model API mode
  solver.enable_model_api();

  // Set optimization sense to maximize
  solver.set_sense(Model_API::Sense::maximize);

  // Set solver parameters
  solver.set_time_limit(1.0);
  solver.set_log_obj(true);

  // Build the following MIP model:
  // Maximize: x1 + 2*x2 + 3*x3 + x4
  // Subject to:
  //   -x1 + x2 + x3 + 10*x4 <= 20
  //    x1 - 3*x2 + x3       <= 30
  //         x2 - 3.5*x4 = 0
  // Bounds:
  //   0 <= x1 <= 40
  //   0 <= x2
  //   0 <= x3
  //   2 <= x4 <= 3
  //   x4 is integer

  // Add variables
  std::cout << "Building model...\n";

  int x1 = solver.add_var("x1", 0.0, 40.0, 1.0, Var_Type::real);
  int x2 = solver.add_var("x2", 0.0, inf, 2.0, Var_Type::real);
  int x3 = solver.add_var("x3", 0.0, inf, 3.0, Var_Type::real);
  int x4 = solver.add_var("x4", 2.0, 3.0, 1.0, Var_Type::general_integer);

  assert(x1 == 0 && x2 == 1 && x3 == 2 && x4 == 3);

  std::cout << "Added 4 variables: x1, x2, x3, x4\n";

  // Add constraints
  // Constraint 1: -x1 + x2 + x3 + 10*x4 <= 20
  solver.add_con(-inf, 20.0, std::vector<int>{x1, x2, x3, x4},
                 std::vector<double>{-1.0, 1.0, 1.0, 10.0});

  // Constraint 2: x1 - 3*x2 + x3 <= 30
  solver.add_con(-inf, 30.0, std::vector<int>{x1, x2, x3},
                 std::vector<double>{1.0, -3.0, 1.0});

  // Constraint 3: x2 - 3.5*x4 = 0
  solver.add_con(0.0, 0.0, std::vector<int>{x2, x4},
                 std::vector<double>{1.0, -3.5});

  std::cout << "Added 3 constraints\n";

  // Alternative way to add constraints using variable names:
  // solver.add_con(-inf, 20.0, std::vector<std::string>{"x1", "x2", "x3", "x4"},
  //                std::vector<double>{-1.0, 1.0, 1.0, 10.0});

  std::cout << "\nModel Summary:\n";
  std::cout << "  Objective: Maximize x1 + 2*x2 + 3*x3 + x4\n";
  std::cout << "  Variables: 4 (3 continuous, 1 integer)\n";
  std::cout << "  Constraints: 3\n";
  std::cout << "  Time limit: 1 seconds\n";

  // Run the solver
  std::cout << "\nStarting solver...\n";
  std::cout << "=====================================\n";
  solver.run();
  std::cout << "=====================================\n";

  // Get results
  std::cout << "\nResults:\n";
  std::cout << "  Objective value: " << solver.get_obj_value() << "\n";
  std::cout << "  Feasible: " << (solver.is_feasible() ? "Yes" : "No") << "\n";

  if (solver.is_feasible())
  {
    const auto& solution = solver.get_solution();
    std::cout << "  Solution:\n";
    std::cout << "    x1 = " << solution[0] << "\n";
    std::cout << "    x2 = " << solution[1] << "\n";
    std::cout << "    x3 = " << solution[2] << "\n";
    std::cout << "    x4 = " << solution[3] << "\n";
  }

  return 0;
}
