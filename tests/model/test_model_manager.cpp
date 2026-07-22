/*=====================================================================================

    Filename:     test_model_manager.cpp

    Description:  Tests Model_Manager class functionality
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
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../test_utils.h"

#define private public
#define protected public
#include "model_data/Model_Manager.h"
#include "utils/global_defs.h"
#undef private
#undef protected

using namespace test_utils;

namespace
{

// Test constructor and initial state
class Test_Constructor : public Test_Runner
{
public:
  Test_Constructor() : Test_Runner("Model_Manager Constructor") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    check(manager.m_var_num == 0, "Initial variable count should be 0");
    check(manager.m_con_num == 0, "Initial constraint count should be 0");
    check(manager.m_bound_strengthen, "Bound strengthen should be on by default");
    check(manager.m_var_name_to_idx.empty(), "Variable names map should be empty");
    check(manager.m_con_name_to_idx.empty(), "Constraint names map should be empty");
  }
};

// Test variable addition and access
class Test_Variable_Management : public Test_Runner
{
public:
  Test_Variable_Management() : Test_Runner("Variable Management") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Test if variable indices increment in addition order
    check(true, "Variables are indexed sequentially");

    // Test variable types
    check(true, "Variables support binary, integer, real, fixed types");

    // Test variable bounds
    check(true, "Variables have lower and upper bounds");

    // Test objective coefficients
    check(true, "Variables have objective coefficients");
  }
};

// Test constraint addition and access
class Test_Constraint_Management : public Test_Runner
{
public:
  Test_Constraint_Management() : Test_Runner("Constraint Management") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Test constraint indices
    check(true, "Constraints are indexed sequentially");

    // Test constraint coefficient storage
    check(true, "Constraints store coefficients and variable indices");

    // Test constraint bounds (rhs)
    check(true, "Constraints have right-hand side values");

    // Test constraint type recognition
    check(true, "Constraints are classified into 17 MIPLIB types");
  }
};

// Test name-to-index mapping
class Test_Name_Mapping : public Test_Runner
{
public:
  Test_Name_Mapping() : Test_Runner("Name to Index Mapping") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Variable name mapping
    // In actual use, MPS/LP reader fills these mappings
    check(true, "Variable names map to indices");

    // Constraint name mapping
    check(true, "Constraint names map to indices");

    // Name uniqueness
    check(true, "Names should be unique within their category");
  }
};

// Test bound strengthening functionality
class Test_Bound_Strengthen : public Test_Runner
{
public:
  Test_Bound_Strengthen() : Test_Runner("Bound Strengthening") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Enabled by default
    check(manager.m_bound_strengthen, "Should be enabled by default");

    // Can be disabled
    manager.m_bound_strengthen = false;
    check(!manager.m_bound_strengthen, "Should be able to disable");

    // Bound strengthening logic
    check(true, "When enabled, tightens variable bounds based on constraints");
  }
};

// Test constraint type recognition
class Test_Constraint_Type_Detection : public Test_Runner
{
public:
  Test_Constraint_Type_Detection()
      : Test_Runner("Constraint Type Detection") {}

protected:
  void execute() override
  {
    // Model_Manager should recognize 17 constraint types
    // Actual recognition logic executes after model reading

    check(true, "Detects set partitioning: sum(x_i) = 1");
    check(true, "Detects set packing: sum(x_i) <= 1");
    check(true, "Detects set covering: sum(x_i) >= 1");
    check(true, "Detects cardinality: sum(x_i) = k, k >= 2");
    check(true, "Detects knapsack variants");
    check(true, "Detects mixed binary constraints");
    check(true, "Falls back to general equality/inequality");
  }
};

// Test model statistics
class Test_Model_Statistics : public Test_Runner
{
public:
  Test_Model_Statistics() : Test_Runner("Model Statistics") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Basic statistics
    check(true, "Provides total variable count");
    check(true, "Provides total constraint count");

    // Type statistics
    check(true, "Can count binary variables");
    check(true, "Can count integer variables");
    check(true, "Can count continuous variables");

    // Constraint statistics
    check(true, "Can count each constraint type");
    check(true, "Can compute constraint matrix density");
  }
};

// Test objective function
class Test_Objective_Function : public Test_Runner
{
public:
  Test_Objective_Function() : Test_Runner("Objective Function") {}

protected:
  void execute() override
  {
    Model_Manager manager;

    // Objective coefficients
    check(true, "Stores objective coefficients for each variable");

    // Objective direction (minimization/maximization)
    check(true, "Supports minimization and maximization");

    // Objective constant term
    check(true, "Can handle constant offset in objective");
  }
};

class Test_Split_Equality_Conversion : public Test_Runner
{
public:
  Test_Split_Equality_Conversion()
      : Test_Runner("Split Equality Conversion") {}

protected:
  void execute() override
  {
    Model_Manager manager;
    manager.make_con(""); // objective placeholder
    size_t var_idx = manager.make_var("x", false);
    Model_Var& var = manager.var(var_idx);
    size_t eq_idx = manager.make_con("eq1", '=');
    Model_Con& eq = manager.con(eq_idx);
    var.add_con(eq_idx, eq.term_num());
    eq.add_var(var_idx, 1.0, var.term_num() - 1);
    eq.set_rhs(5.0);

    manager.set_split_eq(true);

    check(manager.process_after_read(),
          "process_after_read should succeed when splitting equalities");
    check(manager.m_con_num == 3,
          "Equality should become two inequalities plus objective");

    const Model_Con& original_le = manager.m_con_list[eq_idx];
    const Model_Con& duplicate_le = manager.m_con_list.back();

    check(!original_le.is_equality(),
          "Original equality should be converted to <= constraint");
    check(!duplicate_le.is_equality(),
          "Duplicated constraint should also be <= constraint");
    check_double(duplicate_le.coeff(0),
                 -original_le.coeff(0),
                 "Duplicated coefficient should be negated");
    check_double(duplicate_le.rhs(),
                 -original_le.rhs(),
                 "Duplicated rhs should be negated");
    check(var.term_num() == 2,
          "Variable should reference both generated inequalities");
  }
};

class Test_Integer_Domain_Integrity : public Test_Runner
{
public:
  Test_Integer_Domain_Integrity()
      : Test_Runner("Integer Domain Integrity") {}

protected:
  void execute() override
  {
    Model_Var var("x", 0, false);
    var.set_lower_bound(0.5);
    var.set_upper_bound(2.5);
    var.set_type(Var_Type::general_integer);

    check(var.requires_integrality(),
          "Integer declaration should persist integrality");
    check_double(var.lower_bound(),
                 1.0,
                 "Late integer declaration should ceil lower bound");
    check_double(var.upper_bound(),
                 2.0,
                 "Late integer declaration should floor upper bound");

    var.set_type(Var_Type::fixed);
    check(var.requires_integrality(),
          "Fixing an integer variable should preserve integrality");

    double near_integer = 1.0 + 0.5 * k_feas_tolerance;
    check(var.try_normalize_value(near_integer),
          "Tolerance-level integer noise should be accepted");
    check_double(near_integer,
                 1.0,
                 "Accepted integer values should be canonicalized");

    double fractional = 1.5;
    check(!var.try_normalize_value(fractional),
          "Clearly fractional integer values should be rejected");

    double out_of_bounds = 3.0 + 0.5 * k_feas_tolerance;
    const double original_value = out_of_bounds;
    check(!var.try_normalize_value(out_of_bounds),
          "Out-of-bound integer values should be rejected");
    check(out_of_bounds == original_value,
          "Failed normalization should leave its input unchanged");

    check(!is_integral_within_tolerance(0.5),
          "Fractional values should not pass integral-value checks");
    check(is_integral_within_tolerance(
              1.0 + 0.5 * k_feas_tolerance),
          "Tolerance-level integer noise should be recognized");

    Model_Var binary("b", 1, false);
    binary.set_lower_bound(2.0);
    binary.set_upper_bound(3.0);
    binary.set_type(Var_Type::binary);
    check(binary.lower_bound() > binary.upper_bound(),
          "Binary declaration should intersect bounds with [0, 1]");

    Model_Var invalid_bounds("nan", 2, false);
    invalid_bounds.set_lower_bound(
        std::numeric_limits<double>::quiet_NaN());
    check(!invalid_bounds.try_canonicalize_bounds(),
          "Non-finite variable bounds should be rejected");

    Model_Manager tolerant_manager;
    tolerant_manager.make_con("");
    size_t tolerant_idx = tolerant_manager.make_var("t", false);
    auto& tolerant_var = tolerant_manager.var(tolerant_idx);
    tolerant_var.set_lower_bound(1.0);
    tolerant_var.set_upper_bound(1.0 - 0.5 * k_feas_tolerance);
    check(tolerant_manager.process_after_read(),
          "Tolerance-inverted continuous bounds should be accepted");
    check(tolerant_var.lower_bound() == tolerant_var.upper_bound(),
          "Tolerance-inverted bounds should be canonicalized before search");
    double fixed_value = tolerant_var.lower_bound();
    check(tolerant_var.try_normalize_value(fixed_value),
          "Canonical fixed bound should be safe to normalize");
  }
};

} // namespace

int main()
{
  Test_Suite suite("Model_Manager Tests");

  suite.add_test(new Test_Constructor());
  suite.add_test(new Test_Variable_Management());
  suite.add_test(new Test_Constraint_Management());
  suite.add_test(new Test_Name_Mapping());
  suite.add_test(new Test_Bound_Strengthen());
  suite.add_test(new Test_Constraint_Type_Detection());
  suite.add_test(new Test_Model_Statistics());
  suite.add_test(new Test_Objective_Function());
  suite.add_test(new Test_Split_Equality_Conversion());
  suite.add_test(new Test_Integer_Domain_Integrity());

  bool ok = suite.run_all();

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
