/*=====================================================================================

    Filename:     global_defs.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include <cassert>
#include <chrono>
#include <cmath>
#include <stdlib.h>
#include <string>
#include <sys/time.h>

#ifdef DEBUG
#define D_P(...) printf(__VA_ARGS__)
#define DEBUG_CALL(...) (__VA_ARGS__)
#else
#define D_P(...)
#define DEBUG_CALL(...)
#endif

extern std::chrono::steady_clock::time_point g_clk_start;

const double k_inf = 1e20;

const double k_neg_inf = -k_inf;

const double k_default_integer_upper_bound = 1.0;

const double k_default_lower_bound = 0.0;

extern double k_feas_tolerance;

extern double k_opt_tolerance;

extern double k_zero_tolerance;

enum class Var_Type
{
  binary,

  general_integer,

  real,

  fixed
};

enum class Con_Type
{
  empty, // no variables

  free, // no finite side

  singleton, // single variable

  aggregation, // ax + by = c

  precedence, // ax - ay <= b (x and y must have the same variable type)

  var_bound, // ax + by <= c, x ∈ {0,1}

  set_partitioning, // Σx_i = 1, x_i ∈ {0,1}

  set_packing, // Σx_i <= 1, x_i ∈ {0,1}

  set_covering, // Σx_i >= 1 --> Σ-x_i <= -1, x_i ∈ {0,1}

  cardinality, // Σx_i = k, x_i ∈ {0,1}, k ∈ N>= 2

  invariant_knapsack, // Σx_i <= b, x_i ∈ {0,1}, b ∈ N>= 2

  equation_knapsack, // Σa_i x_i = b, x_i ∈ {0,1}, b ∈ N>= 2

  bin_packing, // Σa_ix_i + by <= b, x_i and y ∈ {0,1}, b ∈ N>= 2

  knapsack, // Σa_k x_k <= b, x_i ∈ {0,1}, b ∈ N>= 2

  integer_knapsack, // Σa_k x_k <= b, x_i ∈ Z, b ∈ N

  mixed_binary, // Σa_k x_k + Σp_j s_j {<=,=} b, x_i ∈ {0,1}, s_j ∈ real.

  general_equality, // general linear equality constraint

  general_inequality // general linear inequality constraint (<=)
};

double elapsed_time();

const char* con_type_str(Con_Type p_type);
