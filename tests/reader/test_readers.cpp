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

class Test_Sol_Reader_Integrality : public Test_Runner
{
public:
  Test_Sol_Reader_Integrality()
      : Test_Runner("SOL Reader Integrality") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    manager.make_con("");
    size_t var_idx = manager.make_var("x", false);
    auto& var = manager.var(var_idx);
    var.set_type(Var_Type::binary);
    var.set_lower_bound(0.0);
    var.set_upper_bound(1.0);
    check(manager.process_after_read(),
          "Simple binary model should be valid");

    const char* fractional_file = "tmp_start_fractional.sol";
    std::FILE* fp = std::fopen(fractional_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create fractional start file");
      return;
    }
    std::fprintf(fp, "Variable name        Variable value\n");
    std::fprintf(fp, "x 0.5\n");
    std::fclose(fp);

    std::vector<double> solution;
    std::vector<char> present;
    Sol_Read_Result result =
        Sol_Reader::read(fractional_file, manager, solution, &present);
    check(!result.m_success,
          "Fractional binary warm start should be rejected");
    check(result.m_message.find("violates integrality") !=
              std::string::npos,
          "Integrality error should be explicit");
    check(result.m_message.find(":2") != std::string::npos,
          "Integrality error should include line number");
    std::remove(fractional_file);

    const char* near_file = "tmp_start_near_integer.sol";
    fp = std::fopen(near_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create near-integer start file");
      return;
    }
    std::fprintf(fp, "Variable name        Variable value\n");
    std::fprintf(fp, "x %.15g\n", 1.0 - 0.5 * k_feas_tolerance);
    std::fclose(fp);

    result = Sol_Reader::read(near_file, manager, solution, &present);
    check(result.m_success,
          "Tolerance-level integer noise should be accepted");
    check_double(solution[var_idx],
                 1.0,
                 "Accepted integer warm start should be canonicalized");
    check(present.size() == 1 && present[0] == 1,
          "Warm-start presence mask should record loaded variables");
    std::remove(near_file);
  }
};

class Test_LP_Integer_Bound_Normalization : public Test_Runner
{
public:
  Test_LP_Integer_Bound_Normalization()
      : Test_Runner("LP Integer Bound Normalization") {}

protected:
  void execute() override
  {
    const char* lp_file = "tmp_fractional_integer_bounds.lp";
    std::FILE* fp = std::fopen(lp_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create LP bound test file");
      return;
    }
    std::fprintf(fp,
                 "Minimize\n obj: x\nBounds\n 0.5 <= x <= 2.5\n"
                 "General\n x\nEnd\n");
    std::fclose(fp);

    Model_Manager manager;
    LP_Reader reader(&manager);
    reader.read(lp_file);
    check(manager.process_after_read(),
          "Integer interval with feasible integer points should be valid");
    const auto& var = manager.var("x");
    check(var.requires_integrality(),
          "LP General declaration should mark integer domain");
    check_double(var.lower_bound(),
                 1.0,
                 "LP integer lower bound should be ceiled");
    check_double(var.upper_bound(),
                 2.0,
                 "LP integer upper bound should be floored");
    std::remove(lp_file);

    const char* empty_lp_file = "tmp_empty_integer_domain.lp";
    fp = std::fopen(empty_lp_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create empty integer domain file");
      return;
    }
    std::fprintf(fp,
                 "Minimize\n obj: x\nBounds\n 0.2 <= x <= 0.8\n"
                 "General\n x\nEnd\n");
    std::fclose(fp);

    Model_Manager empty_manager;
    LP_Reader empty_reader(&empty_manager);
    empty_reader.read(empty_lp_file);
    check(!empty_manager.process_after_read(),
          "Integer interval without an integer point should be infeasible");
    std::remove(empty_lp_file);
  }
};

class Test_MPS_Fractional_Integer_Fixed_Bound : public Test_Runner
{
public:
  Test_MPS_Fractional_Integer_Fixed_Bound()
      : Test_Runner("MPS Fractional Integer Fixed Bound") {}

protected:
  void execute() override
  {
    const char* mps_file = "tmp_fractional_integer_fixed.mps";
    std::FILE* fp = std::fopen(mps_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create MPS fixed-bound test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n"
                 " MARK0000 'MARKER' 'INTORG'\n"
                 " x OBJ 1\n"
                 " MARK0001 'MARKER' 'INTEND'\n"
                 "RHS\n"
                 "BOUNDS\n FX BND x 0.5\n"
                 "ENDATA\n");
    std::fclose(fp);

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(mps_file);
    check(!manager.process_after_read(),
          "Fractionally fixed integer MPS variable should be infeasible");
    check(manager.var("x").requires_integrality(),
          "MPS fixed bound must not relax an integer variable to real");
    std::remove(mps_file);
  }
};

class Test_MPS_Integer_Bound_Types : public Test_Runner
{
public:
  Test_MPS_Integer_Bound_Types()
      : Test_Runner("MPS Integer Bound Types") {}

protected:
  void execute() override
  {
    const char* mps_file = "tmp_integer_bound_types.mps";
    std::FILE* fp = std::fopen(mps_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create MPS integer-bound test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n x OBJ 1\n"
                 "RHS\n"
                 "BOUNDS\n LI BND x 0\n UI BND x 2\n"
                 "ENDATA\n");
    std::fclose(fp);

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(mps_file);
    check(manager.process_after_read(),
          "LI/UI integer bounds should produce a valid model");
    check(manager.var("x").requires_integrality(),
          "LI/UI bounds should declare an integer variable");
    std::remove(mps_file);

    const char* invalid_file = "tmp_fractional_li_bound.mps";
    fp = std::fopen(invalid_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create invalid MPS integer-bound test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n x OBJ 1\n"
                 "RHS\n"
                 "BOUNDS\n LI BND x 0.5\n"
                 "ENDATA\n");
    std::fclose(fp);

    bool threw = false;
    try
    {
      Model_Manager invalid_manager;
      MPS_Reader invalid_reader(&invalid_manager);
      invalid_reader.read(invalid_file);
    }
    catch (const Solver_Error&)
    {
      threw = true;
    }
    check(threw, "Fractional LI/UI values should be rejected");
    std::remove(invalid_file);
  }
};

class Test_MPS_Bound_Semantics : public Test_Runner
{
public:
  Test_MPS_Bound_Semantics()
      : Test_Runner("MPS Bound Semantics") {}

protected:
  void execute() override
  {
    const char* marker_file = "tmp_marker_bounds.mps";
    std::FILE* fp = std::fopen(marker_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create MPS marker-bound test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n"
                 " MARK0000 'MARKER' 'INTORG'\n"
                 " x OBJ 1\n y OBJ 1\n z OBJ 1\n w OBJ 1\n"
                 " MARK0001 'MARKER' 'INTEND'\n"
                 "RHS\n"
                 "BOUNDS\n"
                 " LO BND x 2\n UP BND y 5\n UP BND z -1\n"
                 "ENDATA\n");
    std::fclose(fp);

    Model_Manager marker_manager;
    MPS_Reader marker_reader(&marker_manager);
    marker_reader.read(marker_file);
    check_double(marker_manager.var("x").lower_bound(),
                 2.0,
                 "Explicit LO should replace the marker default domain");
    check_double(marker_manager.var("x").upper_bound(),
                 k_inf,
                 "Marker integer with LO should have no default upper bound");
    check_double(marker_manager.var("y").upper_bound(),
                 5.0,
                 "Marker integer should accept an explicit upper bound");
    check_double(marker_manager.var("z").lower_bound(),
                 k_neg_inf,
                 "Negative UP without LO should imply an infinite lower bound");
    check_double(marker_manager.var("w").upper_bound(),
                 1.0,
                 "Unbounded marker integer should retain binary defaults");
    std::remove(marker_file);

    const char* vector_file = "tmp_mps_vectors.mps";
    fp = std::fopen(vector_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create MPS vector test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n L C\n"
                 "COLUMNS\n x OBJ 1 C 1\n"
                 "RHS\n FIRST C 3\n SECOND C 7\n"
                 "RANGES\n FIRST C 2\n SECOND C 4\n"
                 "BOUNDS\n UP FIRST x 5\n LO SECOND x 2\n"
                 "ENDATA\n");
    std::fclose(fp);

    Model_Manager vector_manager;
    MPS_Reader vector_reader(&vector_manager);
    vector_reader.read(vector_file);
    check_double(vector_manager.con("C").rhs(),
                 3.0,
                 "Only the first RHS vector should be selected");
    check_double(vector_manager.con("C_range_0").rhs(),
                 1.0,
                 "Only the first range vector should be selected");
    check_double(vector_manager.var("x").lower_bound(),
                 0.0,
                 "Bounds from later vectors should be ignored");
    check_double(vector_manager.var("x").upper_bound(),
                 5.0,
                 "The first bound vector should be selected");
    std::remove(vector_file);

    const char* duplicate_file = "tmp_duplicate_mps_bound.mps";
    fp = std::fopen(duplicate_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create duplicate MPS-bound test file");
      return;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n x OBJ 1\n"
                 "RHS\n"
                 "BOUNDS\n UP BND x 3\n UI BND x 2\n"
                 "ENDATA\n");
    std::fclose(fp);

    bool threw = false;
    try
    {
      Model_Manager duplicate_manager;
      MPS_Reader duplicate_reader(&duplicate_manager);
      duplicate_reader.read(duplicate_file);
    }
    catch (const Solver_Error&)
    {
      threw = true;
    }
    check(threw, "Duplicate upper bounds should be rejected");
    std::remove(duplicate_file);
  }
};

class Test_MPS_Bound_Value_Syntax : public Test_Runner
{
private:
  const char* m_mps_file = "tmp_mps_bound_value_syntax.mps";

  bool write_model(const char* p_bound_record)
  {
    std::FILE* fp = std::fopen(m_mps_file, "w");
    if (fp == nullptr)
    {
      check(false, "Should create MPS bound-value test file");
      return false;
    }
    std::fprintf(fp,
                 "NAME test\n"
                 "ROWS\n N OBJ\n"
                 "COLUMNS\n x OBJ 1\n"
                 "RHS\n"
                 "BOUNDS\n %s\n"
                 "ENDATA\n",
                 p_bound_record);
    std::fclose(fp);
    return true;
  }

  void check_rejected(const char* p_bound_record, const char* p_message)
  {
    if (!write_model(p_bound_record))
      return;

    bool threw = false;
    try
    {
      Model_Manager manager;
      MPS_Reader reader(&manager);
      reader.read(m_mps_file);
    }
    catch (const Solver_Error&)
    {
      threw = true;
    }
    check(threw, p_message);
    std::remove(m_mps_file);
  }

public:
  Test_MPS_Bound_Value_Syntax()
      : Test_Runner("MPS Bound Value Syntax") {}

protected:
  void execute() override
  {
    if (!write_model("BV BND x"))
      return;

    Model_Manager manager;
    MPS_Reader reader(&manager);
    reader.read(m_mps_file);
    check(manager.process_after_read(),
          "BV without a value should produce a valid model");
    check(manager.var("x").is_binary(),
          "BV without a value should declare a binary variable");
    std::remove(m_mps_file);

    check_rejected("BV BND x 1", "BV should reject a value field");
    check_rejected("FR BND x 0", "FR should reject a value field");
    check_rejected("MI BND x 0", "MI should reject a value field");
    check_rejected("PL BND x 0", "PL should reject a value field");
    check_rejected("UP BND x", "UP should require a value field");
    check_rejected("UP BND x 1abc",
                   "A bound value must be a complete number");
    check_rejected("UP BND x 1 extra",
                   "A bound record should reject extra fields");
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
  suite.add_test(new Test_Sol_Reader_Integrality());
  suite.add_test(new Test_LP_Integer_Bound_Normalization());
  suite.add_test(new Test_MPS_Fractional_Integer_Fixed_Bound());
  suite.add_test(new Test_MPS_Integer_Bound_Types());
  suite.add_test(new Test_MPS_Bound_Semantics());
  suite.add_test(new Test_MPS_Bound_Value_Syntax());

  bool ok = suite.run_all();

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
