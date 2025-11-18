# Start Callback Example

Demonstrates how to customize the initial solution generation using a callback function.

## What It Does

This example shows how to:
- Define a custom start callback to control initial variable values
- Access solver context (variables, RNG, model info)
- Pass user data to the callback
- Track callback invocations

## Why Use Start Callbacks?

Custom initialization strategies can:
- Leverage domain knowledge to create better starting points
- Implement problem-specific heuristics
- Warm-start the solver with partial solutions
- Improve convergence speed

## Code Overview

```cpp
#include "local_mip/Local_MIP.h"

int main()
{
  Local_MIP solver;
  int call_count = 0;

  // Define custom start callback
  Local_Search::Start_Cbk cbk = [](Start::Start_Ctx& ctx, void* p_user_data)
  {
    auto& values = ctx.m_var_current_value;       // Variable values (read/write)
    auto& rng = ctx.m_rng;                        // Random number generator
    const auto& binary_vars = ctx.m_shared.m_binary_idx_list;

    // Track callback invocations using user_data
    if (p_user_data)
    {
      int* counter = static_cast<int*>(p_user_data);
      (*counter)++;
    }

    // Randomly initialize binary variables
    std::uniform_int_distribution<int> dist(0, 1);
    for (size_t var_idx : binary_vars)
    {
      values[var_idx] = dist(rng);
    }
  };

  // Register callback with user data
  solver.set_start_cbk(cbk, &call_count);

  solver.set_model_file("test-set/2club200v15p5scn.mps");
  solver.set_time_limit(60.0);
  solver.run();

  return 0;
}
```

## Context API

The callback receives `Start::Start_Ctx` with access to:

| Member | Type | Access | Description |
|--------|------|--------|-------------|
| `m_var_current_value` | `vector<double>&` | R/W | Variable values to initialize |
| `m_rng` | `mt19937&` | R/W | Random number generator |
| `m_shared.m_binary_idx_list` | `vector<size_t>` | R | Binary variable indices |
| `m_shared.m_model_manager` | `Model_Manager&` | R | Model metadata |

## Building & Running

```bash
cd example/start-callback
g++ -O3 -std=c++20 start_callback.cpp -I../../src -L../../build -lLocalMIP -lpthread -o start_callback_demo
cd ..  # run from example/ so test-set/ is in place
./start-callback/start_callback_demo
```

## Expected Output

```
c custom start callback is registered.
c model name: 2club200v15p5scn
...
Callback called 1 time(s)
Callback: Randomly initialized 200 binary variables
c [      0.10] obj*: -31
c [      1.30] obj*: -60
...
o best objective: -67
Solution is feasible!
Objective value: -67.0000000000
```

## Key Points

1. **Callback Signature**: `void callback(Start::Start_Ctx& ctx, void* user_data)`
2. **Invocation Timing**: Called once after default initialization (zero/random)
3. **Variable Access**: Modify `ctx.m_var_current_value[idx]` to set initial values
4. **User Data**: Optional pointer for passing custom data to callback
5. **Thread Safety**: Callback is called in the solver's main thread

## Advanced Usage

See also:
- `restart-callback/` - Custom restart strategies
- `weight-callback/` - Custom constraint weight updates
- `neighbor-userdata/` - Passing complex user data structures
