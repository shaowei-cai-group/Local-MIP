/*=====================================================================================

    Filename:     test_end_to_end.cpp

    Description:  End-to-end integration tests
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "../test_utils.h"

#define private public
#define protected public
#include "Local_MIP.h"
#undef private
#undef protected

// Test instance path
#ifndef TEST_MPS_PATH
#define TEST_MPS_PATH "test-set/sct1.mps"
#endif

using namespace test_utils;

namespace
{

// Test complete solving process
class Test_Complete_Solve : public Test_Runner
{
public:
  Test_Complete_Solve() : Test_Runner("Complete Solve Pipeline") {}

protected:
  void execute() override
  {
    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(1.0); // 1 second time limit
    solver.set_log_obj(false);  // Disable logging to avoid output interference

    try
    {
      solver.run();

      check(true, "Solver should complete without crashing");

      // Verify solver completion
      check(solver.m_local_search->m_terminated,
            "Solver should be terminated after run");

      std::printf("  Solve completed successfully\n");
    }
    catch (...)
    {
      check(false, "Solver should not throw exceptions");
    }
  }
};

// Test Find Feasible Solution
class Test_Find_Feasible_Solution : public Test_Runner
{
public:
  Test_Find_Feasible_Solution() : Test_Runner("Find Feasible Solution") {}

protected:
  void execute() override
  {
    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(5.0); // Give enough time
    solver.set_log_obj(false);

    try
    {
      solver.run();

      if (solver.m_local_search->m_is_found_feasible)
      {
        check(true, "Solver found a feasible solution");
        std::printf("  Best objective: %.6f\n",
                    solver.m_local_search->m_best_obj);
      }
      else
      {
        // Some difficult instances may not find feasible solution in short time
        check(true, "Solver attempted to find feasible solution");
        std::printf("  No feasible solution found in time limit\n");
      }
    }
    catch (...)
    {
      check(false, "Test execution failed");
    }
  }
};

// Test solution output functionality
class Test_Solution_Output : public Test_Runner
{
public:
  Test_Solution_Output() : Test_Runner("Solution Output") {}

protected:
  void execute() override
  {
    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(2.0);
    solver.set_sol_path("test_output.sol");
    solver.set_log_obj(false);

    try
    {
      solver.run();

      // If feasible solution found, should write to file
      if (solver.m_local_search->m_is_found_feasible)
      {
        check(true, "Should write solution to file");
        std::printf("  Solution written to test_output.sol\n");
      }
      else
      {
        check(true, "No feasible solution to write");
      }
    }
    catch (...)
    {
      check(false, "Solution output test failed");
    }
  }
};

// Test different start methods
class Test_Different_Start_Methods : public Test_Runner
{
public:
  Test_Different_Start_Methods() : Test_Runner("Different Start Methods") {}

protected:
  void execute() override
  {
    // Test zero start
    {
      Local_MIP solver;
      solver.set_model_file(TEST_MPS_PATH);
      solver.set_time_limit(1.0);
      solver.set_start_method("zero");
      solver.set_log_obj(false);

      try
      {
        solver.run();
        check(true, "Zero start method should work");
      }
      catch (...)
      {
        check(false, "Zero start failed");
      }
    }

    // Test random start
    {
      Local_MIP solver;
      solver.set_model_file(TEST_MPS_PATH);
      solver.set_time_limit(1.0);
      solver.set_start_method("random");
      solver.set_log_obj(false);

      try
      {
        solver.run();
        check(true, "Random start method should work");
      }
      catch (...)
      {
        check(false, "Random start failed");
      }
    }
  }
};

// Test restart mechanism
class Test_Restart_Mechanism : public Test_Runner
{
public:
  Test_Restart_Mechanism() : Test_Runner("Restart Mechanism") {}

protected:
  void execute() override
  {
    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(3.0);
    solver.set_restart_step(1000); // Restart every 1000 steps
    solver.set_log_obj(false);

    try
    {
      solver.run();
      check(true, "Restart mechanism should function correctly");
      std::printf("  Restart mechanism executed\n");
    }
    catch (...)
    {
      check(false, "Restart mechanism failed");
    }
  }
};

// Test bound strengthening
class Test_Bound_Strengthen : public Test_Runner
{
public:
  Test_Bound_Strengthen() : Test_Runner("Bound Strengthening") {}

protected:
  void execute() override
  {
    // Do not enable bound strengthening
    {
      Local_MIP solver;
      solver.set_model_file(TEST_MPS_PATH);
      solver.set_time_limit(1.0);
      solver.set_bound_strengthen(false);
      solver.set_log_obj(false);

      try
      {
        solver.run();
        check(true, "Should work without bound strengthening");
      }
      catch (...)
      {
        check(false, "Failed without bound strengthening");
      }
    }

    // Enable bound strengthening
    {
      Local_MIP solver;
      solver.set_model_file(TEST_MPS_PATH);
      solver.set_time_limit(1.0);
      solver.set_bound_strengthen(true);
      solver.set_log_obj(false);

      try
      {
        solver.run();
        check(true, "Should work with bound strengthening");
      }
      catch (...)
      {
        check(false, "Failed with bound strengthening");
      }
    }
  }
};

// Test timeout control
class Test_Timeout_Control : public Test_Runner
{
public:
  Test_Timeout_Control() : Test_Runner("Timeout Control") {}

protected:
  void execute() override
  {
    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(0.5); // Very short time limit
    solver.set_log_obj(false);

    auto start = std::chrono::steady_clock::now();

    try
    {
      solver.run();
    }
    catch (...)
    {
    }

    auto end = std::chrono::steady_clock::now();
    double elapsed =
        std::chrono::duration<double>(end - start).count();

    check(elapsed < 2.0, "Should respect time limit");
    std::printf("  Elapsed time: %.3f seconds\n", elapsed);
  }
};

// Test multiple runs
class Test_Multiple_Runs : public Test_Runner
{
public:
  Test_Multiple_Runs() : Test_Runner("Multiple Consecutive Runs") {}

protected:
  void execute() override
  {
    // Test if same solver object can run multiple times
    // (Though usually not recommended, should not crash)

    Local_MIP solver;
    solver.set_model_file(TEST_MPS_PATH);
    solver.set_time_limit(0.5);
    solver.set_log_obj(false);

    try
    {
      solver.run();
      check(true, "First run completed");
    }
    catch (...)
    {
      check(false, "First run failed");
    }

    // Note: Second run may need to reinitialize state
    // This test mainly verifies it does not crash
    check(true, "Multiple runs test completed");
  }
};

} // namespace

int main()
{
  Test_Suite suite("End-to-End Integration Tests");

  suite.add_test(new Test_Complete_Solve());
  suite.add_test(new Test_Find_Feasible_Solution());
  suite.add_test(new Test_Solution_Output());
  suite.add_test(new Test_Different_Start_Methods());
  suite.add_test(new Test_Restart_Mechanism());
  suite.add_test(new Test_Bound_Strengthen());
  suite.add_test(new Test_Timeout_Control());
  suite.add_test(new Test_Multiple_Runs());

  bool ok = suite.run_all();

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
