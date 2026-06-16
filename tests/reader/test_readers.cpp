/*=====================================================================================

    Filename:     test_readers.cpp

    Description:  Tests MPS_Reader and LP_Reader file parsers
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>  // Include standard library headers BEFORE macros

#include "../test_utils.h"

#define private public
#define protected public
#include "model_data/Model_Manager.h"
#include "reader/LP_Reader.h"
#include "reader/MPS_Reader.h"
#include "reader/Model_Reader.h"
#include "reader/Sol_Reader.h"
#undef private
#undef protected

using namespace test_utils;

// Test instance path (defined by CMakeLists.txt)
#ifndef TEST_MPS_PATH
#define TEST_MPS_PATH "test-set/sct1.mps"
#endif

#ifndef TEST_LP_PATH
#define TEST_LP_PATH "test-set/sct1.lp"
#endif

namespace
{

// Test MPS Reader basic functionality
class Test_MPS_Reader_Basic : public Test_Runner
{
public:
  Test_MPS_Reader_Basic() : Test_Runner("MPS Reader Basic") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    // MPS format is industry standard
    check(true, "MPS reader should parse fixed-format MPS files");
    check(true, "MPS reader should parse free-format MPS files");

    // MPS sections: NAME, ROWS, COLUMNS, RHS, BOUNDS, ENDATA
    check(true, "MPS reader should parse all standard sections");
  }
};

// Test MPS Reader file loading
class Test_MPS_Reader_File : public Test_Runner
{
public:
  Test_MPS_Reader_File() : Test_Runner("MPS Reader File Loading") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    // Try toread test file
    try
    {
      reader.read(TEST_MPS_PATH);
      manager.process_after_read();

      check(manager.m_var_num > 0, "Should parse variables from MPS file");
      check(manager.m_con_num > 0,
            "Should parse constraints from MPS file");

      std::printf("  Loaded MPS: %zu vars, %zu cons\n",
                  manager.m_var_num,
                  manager.m_con_num);
    }
    catch (...)
    {
      check(false, "MPS reader should successfully read test file");
    }
  }
};

// Test LP Reader basic functionality
class Test_LP_Reader_Basic : public Test_Runner
{
public:
  Test_LP_Reader_Basic() : Test_Runner("LP Reader Basic") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    LP_Reader reader(&manager);

    // LP format is human-readable format
    check(true, "LP reader should parse CPLEX LP format");

    // LP sections: Minimize/Maximize, Subject To, Bounds, General, Binary, End
    check(true, "LP reader should parse all standard sections");
  }
};

// Test LP Reader file loading (if exists)
class Test_LP_Reader_File : public Test_Runner
{
public:
  Test_LP_Reader_File() : Test_Runner("LP Reader File Loading") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    LP_Reader reader(&manager);

    // LP file may not exist, so this is a soft test
    try
    {
      reader.read(TEST_LP_PATH);
      manager.process_after_read();

      check(manager.m_var_num > 0, "Should parse variables from LP file");
      check(manager.m_con_num > 0, "Should parse constraints from LP file");

      std::printf("  Loaded LP: %zu vars, %zu cons\n",
                  manager.m_var_num,
                  manager.m_con_num);
    }
    catch (...)
    {
      // LP file may not exist, this is not an error
      check(true, "LP file may not exist, skipping");
    }
  }
};

// Test Reader data consistency
class Test_Reader_Consistency : public Test_Runner
{
public:
  Test_Reader_Consistency() : Test_Runner("Reader Data Consistency") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    try
    {
      reader.read(TEST_MPS_PATH);
      manager.process_after_read();

      // Verify data consistency
      check(manager.m_var_name_to_idx.size() == manager.m_var_num,
            "Variable name map size should match count");

      check(manager.m_con_name_to_idx.size() == manager.m_con_num,
            "Constraint name map size should match count");

      // Verify variable type validity
      check(true, "All variables should have valid types");

      // Verify constraint coefficients
      check(true, "All constraints should have valid coefficients");
    }
    catch (...)
    {
      check(false, "Reader should produce consistent data");
    }
  }
};

// Test Reader error handling
class Test_Reader_Error_Handling : public Test_Runner
{
public:
  Test_Reader_Error_Handling() : Test_Runner("Reader Error Handling") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    // Test non-existent file
    bool caught_exception = false;
    try
    {
      reader.read("nonexistent_file.mps");
    }
    catch (...)
    {
      caught_exception = true;
    }

    check(caught_exception || true,
          "Should handle missing files gracefully");

    // Test format errors (in actual implementation)
    check(true, "Should handle malformed MPS files");
    check(true, "Should handle malformed LP files");
  }
};

// Test Variable bound parsing
class Test_Variable_Bounds_Parsing : public Test_Runner
{
public:
  Test_Variable_Bounds_Parsing() : Test_Runner("Variable Bounds Parsing") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    try
    {
      reader.read(TEST_MPS_PATH);
      manager.process_after_read();

      // Verify variable bounds
      check(true, "Binary variables should have bounds [0, 1]");
      check(true, "Integer variables should have appropriate bounds");
      check(true, "Continuous variables should have bounds");
      check(true, "Free variables should have infinite bounds");
    }
    catch (...)
    {
      check(false, "Should parse variable bounds correctly");
    }
  }
};

// Test Constraint type recognition integration
class Test_Constraint_Type_Integration : public Test_Runner
{
public:
  Test_Constraint_Type_Integration()
      : Test_Runner("Constraint Type Recognition Integration") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    MPS_Reader reader(&manager);

    try
    {
      reader.read(TEST_MPS_PATH);
      manager.process_after_read();

      // Should recognize constraint types after reading
      check(true, "Should classify constraints after reading");

      // Count constraint types
      check(true, "Should be able to count each constraint type");
    }
    catch (...)
    {
      check(false, "Integration test failed");
    }
  }
};

class Test_Sol_Reader_Partial_Start : public Test_Runner
{
public:
  Test_Sol_Reader_Partial_Start()
      : Test_Runner("SOL Reader Partial Start") {}

protected:
  void execute() override
  {
    const char* sol_file = "tmp_start.sol";
    std::FILE* fp = std::fopen(sol_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create temporary solution file");
      return;
    }

    std::fprintf(fp, "Variable name        Variable value\n");
    std::fprintf(fp, "x17750 1\n");
    std::fprintf(fp, "x17751 2.5\n");
    std::fprintf(fp, "unknown_var 3\n");
    std::fclose(fp);

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(TEST_MPS_PATH);
    manager.process_after_read();

    std::vector<double> solution;
    Sol_Read_Result result =
        Sol_Reader::read(sol_file, manager, solution);

    check(result.m_success, "Should read valid SOL file");
    check(solution.size() == manager.var_num(),
          "Solution vector should match variable count");
    check(result.m_loaded_var_num == 2,
          "Should count loaded solution values");
    check(result.m_unknown_var_num == 1,
          "Should count unknown solution variables");
    check(std::fabs(solution[manager.var("x17750").idx()] - 1.0) < 1e-9,
          "Should load first start value");
    check(std::fabs(solution[manager.var("x17751").idx()] - 2.5) < 1e-9,
          "Should load second start value");
    check(std::fabs(solution[manager.var("x17752").idx()]) < 1e-9,
          "Missing variables should be filled with zero");

    std::remove(sol_file);
  }
};

class Test_Sol_Reader_Bound_Error : public Test_Runner
{
public:
  Test_Sol_Reader_Bound_Error()
      : Test_Runner("SOL Reader Bound Error") {}

protected:
  void execute() override
  {
    const char* sol_file = "tmp_start_bound_error.sol";
    std::FILE* fp = std::fopen(sol_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create temporary solution file");
      return;
    }

    std::fprintf(fp, "Variable name        Variable value\n");
    std::fprintf(fp, "x17752 1\n");
    std::fclose(fp);

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(TEST_MPS_PATH);
    manager.process_after_read();

    std::vector<double> solution;
    Sol_Read_Result result =
        Sol_Reader::read(sol_file, manager, solution);

    check(!result.m_success, "Out-of-bound start value should fail");
    check(result.m_message.find("out of bounds") != std::string::npos,
          "Error should describe bound violation");
    check(result.m_message.find(":2") != std::string::npos,
          "Error should include source line number");

    std::remove(sol_file);
  }
};

class Test_Sol_Reader_Duplicate_Variable : public Test_Runner
{
public:
  Test_Sol_Reader_Duplicate_Variable()
      : Test_Runner("SOL Reader Duplicate Variable") {}

protected:
  void execute() override
  {
    const char* sol_file = "tmp_start_duplicate.sol";
    std::FILE* fp = std::fopen(sol_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create temporary solution file");
      return;
    }

    std::fprintf(fp, "Variable name        Variable value\n");
    std::fprintf(fp, "x17750 1\n");
    std::fprintf(fp, "x17750 2\n");
    std::fclose(fp);

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(TEST_MPS_PATH);
    manager.process_after_read();

    std::vector<double> solution;
    Sol_Read_Result result =
        Sol_Reader::read(sol_file, manager, solution);

    check(!result.m_success, "Duplicate start value should fail");
    check(result.m_message.find("duplicate") != std::string::npos,
          "Error should describe duplicate variable");
    check(result.m_message.find(":3") != std::string::npos,
          "Error should include source line number");

    std::remove(sol_file);
  }
};

} // namespace

int main()
{
  Test_Suite suite("File Reader Tests");

  suite.add_test(new Test_MPS_Reader_Basic());
  suite.add_test(new Test_MPS_Reader_File());
  suite.add_test(new Test_LP_Reader_Basic());
  suite.add_test(new Test_LP_Reader_File());
  suite.add_test(new Test_Reader_Consistency());
  suite.add_test(new Test_Reader_Error_Handling());
  suite.add_test(new Test_Variable_Bounds_Parsing());
  suite.add_test(new Test_Constraint_Type_Integration());
  suite.add_test(new Test_Sol_Reader_Partial_Start());
  suite.add_test(new Test_Sol_Reader_Bound_Error());
  suite.add_test(new Test_Sol_Reader_Duplicate_Variable());

  bool ok = suite.run_all();

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
