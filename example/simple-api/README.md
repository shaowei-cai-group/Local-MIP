# Simple API Example

A minimal example demonstrating the basic usage of Local-MIP solver.

## What It Does

This example shows how to:
- Create a solver instance
- Load a MIP problem from file
- Set basic parameters (time limit, solution output)
- Run the solver
- Retrieve solution results

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;

  // Configure solver
  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_sol_path("example_simple.sol");
  solver.set_time_limit(60.0);
  solver.set_log_obj(true);

  // Run solver
  solver.run();

  // Check results
  if (solver.is_feasible())
  {
    printf("Objective value: %.10f\n", solver.get_obj_value());
  }

  return 0;
}
```

## Building

From the project root directory:

```bash
cd example/simple-api
g++ -O3 -std=c++20 simple_api.cpp -I../../src -L../../build -lLocalMIP -lpthread -o simple_api_demo
```

Or use the example build script if available.

## Running

```bash
./simple_api_demo
```

## Expected Output

```
c model name: 2club200v15p5scn
c reading mps file takes 0.14 seconds.
c original problem has 200 variables and 17013 constraints
...
c [     60.00] local search is terminated by timeout.
o best objective: -69
Solution is feasible!
Objective value: -69.0000000000
Solution written to: example_simple.sol
```

## Key API Methods

| Method | Description |
|--------|-------------|
| `set_model_file(path)` | Load MPS/LP file |
| `set_time_limit(seconds)` | Set time limit in seconds |
| `set_sol_path(path)` | Specify solution output file |
| `set_log_obj(true/false)` | Enable objective value logging |
| `run()` | Start solving |
| `is_feasible()` | Check if feasible solution found |
| `get_obj_value()` | Get objective value |

## Next Steps

See other examples for advanced features:
- `start-callback/` - Custom initialization strategies
- `restart-callback/` - Custom restart strategies
- `weight-callback/` - Custom weight update methods
- `neighbor-config/` - Custom neighborhood operations
