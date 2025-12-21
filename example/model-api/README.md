# Model API Example

This example demonstrates how to use Local-MIP's Model API to programmatically build mixed-integer programming models.

## What It Does

The Model API provides a modeling interface similar to solvers like HiGHS and SCIP, allowing users to:

- Add variables and constraints programmatically
- Set objective functions
- Specify variable types (continuous, integer, binary)
- Build complex MIP models flexibly

## Key APIs

### Model Building

- `enable_model_api()` — Enable the modeling API (must be called first)
- `set_sense(Sense)` — Set optimization direction (`maximize` or `minimize`)
- `set_obj_offset(double)` — Set objective function offset
- `add_var(name, lb, ub, cost, type)` — Add a variable
- `set_cost(col/name, cost)` — Modify variable objective coefficient
- `add_con(lb, ub, cols/names, coefs)` — Add a constraint
- `add_var_to_con(row, col/name, coef)` — Add variable to existing constraint
- `set_integrality(col/name, type)` — Modify variable type

### Solver Settings

- `set_time_limit(seconds)` — Set time limit
- `set_log_obj(bool)` — Enable/disable objective value logging
- `set_bound_strengthen(level)` — Set bound strengthening level
- `set_random_seed(seed)` — Set random seed

### Result Query

- `run()` — Run the solver
- `is_feasible()` — Check if a feasible solution was found
- `get_obj_value()` — Get objective function value
- `get_solution()` — Get solution vector

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"
#include "model_api/Model_API.h"
#include <iostream>
#include <vector>

int main()
{
  const double inf = std::numeric_limits<double>::infinity();
  
  // Create solver and enable Model API
  Local_MIP solver;
  solver.enable_model_api();
  
  // Set optimization sense
  solver.set_sense(Model_API::Sense::maximize);
  
  // Set solver parameters
  solver.set_time_limit(1.0);
  solver.set_log_obj(true);
  
  // Add variables
  int x1 = solver.add_var("x1", 0.0, 40.0, 1.0, Var_Type::real);
  int x2 = solver.add_var("x2", 0.0, inf, 2.0, Var_Type::real);
  int x3 = solver.add_var("x3", 0.0, inf, 3.0, Var_Type::real);
  int x4 = solver.add_var("x4", 2.0, 3.0, 1.0, Var_Type::general_integer);
  
  // Add constraints using variable indices
  // Constraint: -x1 + x2 + x3 + 10*x4 <= 20
  solver.add_con(-inf, 20.0,
                 std::vector<int>{x1, x2, x3, x4},
                 std::vector<double>{-1.0, 1.0, 1.0, 10.0});
  
  // Constraint: x1 - 3*x2 + x3 <= 30
  solver.add_con(-inf, 30.0,
                 std::vector<int>{x1, x2, x3},
                 std::vector<double>{1.0, -3.0, 1.0});
  
  // Equality constraint: x2 - 3.5*x4 = 0
  solver.add_con(0.0, 0.0,
                 std::vector<int>{x2, x4},
                 std::vector<double>{1.0, -3.5});
  
  // Alternative: Add constraints using variable names
  // solver.add_con(-inf, 20.0,
  //                std::vector<std::string>{"x1", "x2", "x3", "x4"},
  //                std::vector<double>{-1.0, 1.0, 1.0, 10.0});
  
  // Run solver
  solver.run();
  
  // Get results
  if (solver.is_feasible())
  {
    double obj = solver.get_obj_value();
    const auto& solution = solver.get_solution();
    
    std::cout << "Objective value: " << obj << "\n";
    std::cout << "x1 = " << solution[0] << "\n";
    std::cout << "x2 = " << solution[1] << "\n";
    std::cout << "x3 = " << solution[2] << "\n";
    std::cout << "x4 = " << solution[3] << "\n";
  }
  
  return 0;
}
```

## Variable Types

- `Var_Type::real` — Continuous variable
- `Var_Type::general_integer` — General integer variable
- `Var_Type::binary` — Binary variable (0-1)

## Constraint Bounds

- `[lb, ub]` means `lb <= expression <= ub`
- Use `std::numeric_limits<double>::infinity()` for unbounded bounds
- Equality constraints: set `lb == ub`

## Example Model

The `model_api_demo.cpp` implements the following MIP model:

```
Maximize: x1 + 2*x2 + 3*x3 + x4

Subject to:
  -x1 + x2 + x3 + 10*x4 <= 20
   x1 - 3*x2 + x3       <= 30
        x2 - 3.5*x4 = 0

Bounds:
  0 <= x1 <= 40
  0 <= x2
  0 <= x3
  2 <= x4 <= 3

Variable types:
  x1, x2, x3: continuous
  x4: integer
```

## Build & Run

Build all examples from project root:

```bash
cd example
./build.sh
```

Build/run this example only (requires main project built to provide `build/libLocalMIP.a`):

```bash
cd example/model-api
g++ -O3 -std=c++20 model_api_demo.cpp -I../../src -L../../build -lLocalMIP -lpthread -o model_api_demo
./model_api_demo
```

## Expected Output

```
c Model API enabled for programmatic model building
c time limit is set to : 1.00 seconds
c log obj is set to : true
Building model...
Added 4 variables: x1, x2, x3, x4
Added 3 constraints

Model Summary:
  Objective: Maximize x1 + 2*x2 + 3*x3 + x4
  Variables: 4 (3 continuous, 1 integer)
  Constraints: 3
  Time limit: 1 seconds

Starting solver...
=====================================
c Building model from API...
Building model with 4 variables and 3 constraints...
Model built successfully.
...
o best objective: 23

Results:
  Objective value: 23
  Feasible: Yes
  Solution:
    x1 = 7
    x2 = 7
    x3 = 0
    x4 = 2
```

## Key Points

1. **Must call `enable_model_api()` first** before using any modeling API methods
2. **Variable indices** start from 0 and increment sequentially
3. **Constraint bounds**: `[lb, ub]` represents `lb <= expr <= ub`
4. **Unbounded bounds**: Use `std::numeric_limits<double>::infinity()`
5. **Equality constraints**: Set `lb == ub` to create equality constraints

## Related Examples

- `simple-api/` — Basic solver usage
- `neighbor-config/` — Custom neighborhood configuration
- `start-callback/` — Custom initialization
- `restart-callback/` — Custom restart strategies
