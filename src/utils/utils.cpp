/*=====================================================================================

    Filename:     utils.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "global_defs.h"
#include <cctype>
#include <chrono>
#include <cstdio>

std::chrono::steady_clock::time_point g_clk_start;

double k_feas_tolerance = 1e-6;
double k_opt_tolerance = 1e-4;
double k_zero_tolerance = 1e-9;

double elapsed_time()
{
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::steady_clock::now() - g_clk_start)
      .count();
}

const char* con_type_str(Con_Type type)
{
  switch (type)
  {
    case Con_Type::empty:
      return "Empty";
    case Con_Type::free:
      return "Free";
    case Con_Type::singleton:
      return "Sing.";
    case Con_Type::aggregation:
      return "Agg.";
    case Con_Type::precedence:
      return "Precedence";
    case Con_Type::var_bound:
      return "Var. Bound";
    case Con_Type::set_partitioning:
      return "Set Part.";
    case Con_Type::set_packing:
      return "Set Pack.";
    case Con_Type::set_covering:
      return "Set Cover.";
    case Con_Type::cardinality:
      return "Cardinality";
    case Con_Type::invariant_knapsack:
      return "Inv. Knaps.";
    case Con_Type::equation_knapsack:
      return "Eq. Knaps.";
    case Con_Type::bin_packing:
      return "Bin. Pack.";
    case Con_Type::knapsack:
      return "Knaps.";
    case Con_Type::integer_knapsack:
      return "Int. Knaps.";
    case Con_Type::mixed_binary:
      return "Mixed Bin.";
    case Con_Type::general_equality:
      return "Eq.";
    case Con_Type::general_inequality:
      return "Ineq.";
    default:
      return "Unknown";
  }
}
