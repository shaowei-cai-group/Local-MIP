/*=====================================================================================

    Filename:     test_utils.h

    Description:  Test utility classes and helper functions
        Version:  1.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace test_utils
{

// Check condition and report error
inline bool check(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "ERROR: %s\n", message);
    return false;
  }
  return true;
}

// Floating-point comparison
inline bool double_equal(double a, double b, double tolerance = 1e-9)
{
  return std::fabs(a - b) < tolerance;
}

// Check floating-point equality
inline bool check_double(double actual,
                         double expected,
                         const char* message,
                         double tolerance = 1e-9)
{
  if (!double_equal(actual, expected, tolerance))
  {
    std::fprintf(stderr,
                 "ERROR: %s - expected %.10f, got %.10f\n",
                 message,
                 expected,
                 actual);
    return false;
  }
  return true;
}

// Test runner base class
class Test_Runner
{
public:
  Test_Runner(const char* p_test_name) : m_test_name(p_test_name), m_ok(true) {}

  bool run()
  {
    std::printf("Running: %s\n", m_test_name);
    execute();
    if (m_ok)
    {
      std::printf("  [PASS] %s\n", m_test_name);
    }
    else
    {
      std::fprintf(stderr, "  [FAIL] %s\n", m_test_name);
    }
    return m_ok;
  }

  virtual ~Test_Runner() = default;

protected:
  virtual void execute() = 0;

  bool check(bool condition, const char* message)
  {
    m_ok &= test_utils::check(condition, message);
    return m_ok;
  }

  bool check_double(double actual,
                    double expected,
                    const char* message,
                    double tolerance = 1e-9)
  {
    m_ok &= test_utils::check_double(actual, expected, message, tolerance);
    return m_ok;
  }

  const char* m_test_name;
  bool m_ok;
};

// Test suite
class Test_Suite
{
public:
  Test_Suite(const char* p_suite_name) : m_suite_name(p_suite_name) {}

  void add_test(Test_Runner* p_test) { m_tests.push_back(p_test); }

  bool run_all()
  {
    std::printf("\n========================================\n");
    std::printf("Test Suite: %s\n", m_suite_name);
    std::printf("========================================\n");

    bool all_ok = true;
    for (auto* test : m_tests)
    {
      all_ok &= test->run();
    }

    std::printf("----------------------------------------\n");
    if (all_ok)
    {
      std::printf("All tests PASSED in %s\n", m_suite_name);
    }
    else
    {
      std::fprintf(stderr, "Some tests FAILED in %s\n", m_suite_name);
    }
    std::printf("========================================\n\n");

    return all_ok;
  }

  ~Test_Suite()
  {
    for (auto* test : m_tests)
    {
      delete test;
    }
  }

private:
  const char* m_suite_name;
  std::vector<Test_Runner*> m_tests;
};

} // namespace test_utils
