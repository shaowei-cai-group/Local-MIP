/*=====================================================================================

    Filename:     paras.h

    Description:  Parameter management and command line parsing interface
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include <cstddef>
#include <cstring>
#include <string>

//        name          , type  , short-name, must-need, default, low, high, comments
#define PARAS                                                             \
  PARA(time_limit,                                                        \
       double,                                                            \
       't',                                                               \
       false,                                                             \
       10,                                                                \
       0,                                                                 \
       1e8,                                                               \
       "Time limit in seconds")                                           \
  PARA(random_seed,                                                       \
       int,                                                               \
       'S',                                                               \
       false,                                                             \
       0,                                                                 \
       0,                                                                 \
       2147483647,                                                        \
       "Random seed for local search (0 to use default)")                 \
  PARA(feas_tolerance,                                                    \
       double,                                                            \
       'F',                                                               \
       false,                                                             \
       1e-6,                                                              \
       0,                                                                 \
       1e-2,                                                              \
       "Feasibility tolerance")                                           \
  PARA(opt_tolerance,                                                     \
       double,                                                            \
       'O',                                                               \
       false,                                                             \
       1e-4,                                                              \
       0,                                                                 \
       1,                                                                 \
       "Optimality tolerance")                                            \
  PARA(zero_tolerance,                                                    \
       double,                                                            \
       'Z',                                                               \
       false,                                                             \
       1e-9,                                                              \
       0,                                                                 \
       1e-3,                                                              \
       "Zero value tolerance")                                            \
  PARA(bound_strengthen,                                                  \
       int,                                                               \
       'b',                                                               \
       false,                                                             \
       1,                                                                 \
       0,                                                                 \
       2,                                                                 \
       "Bound strengthen level: 0-off, 1-ip, 2-mip")                      \
  PARA(log_obj, int, 'l', false, 1, 0, 1, "Log objective or not")         \
  PARA(restart_step,                                                      \
       int,                                                               \
       'r',                                                               \
       false,                                                             \
       1000000,                                                           \
       0,                                                                 \
       100000000,                                                         \
       "No-improvement steps before restart (0 disables)")                \
  PARA(smooth_prob,                                                       \
       int,                                                               \
       '0',                                                               \
       false,                                                             \
       1,                                                                 \
       0,                                                                 \
       10000,                                                             \
       "Weight smooth probability in 1/10000")                            \
  PARA(bms_unsat_con,                                                     \
       int,                                                               \
       'u',                                                               \
       false,                                                             \
       12,                                                                \
       0,                                                                 \
       100000000,                                                         \
       "BMS unsatisfied constraint sample size")                          \
  PARA(bms_unsat_ops,                                                     \
       int,                                                               \
       'p',                                                               \
       false,                                                             \
       2250,                                                              \
       0,                                                                 \
       100000000,                                                         \
       "BMS MTM unsatisfied operations")                                  \
  PARA(bms_sat_con,                                                       \
       int,                                                               \
       'v',                                                               \
       false,                                                             \
       1,                                                                 \
       0,                                                                 \
       100000000,                                                         \
       "BMS satisfied constraint sample size")                            \
  PARA(bms_sat_ops,                                                       \
       int,                                                               \
       'o',                                                               \
       false,                                                             \
       80,                                                                \
       0,                                                                 \
       100000000,                                                         \
       "BMS MTM satisfied operations")                                    \
  PARA(bms_flip_ops,                                                      \
       int,                                                               \
       'x',                                                               \
       false,                                                             \
       0,                                                                 \
       0,                                                                 \
       100000000,                                                         \
       "BMS flip operations")                                             \
  PARA(bms_easy_ops,                                                      \
       int,                                                               \
       'q',                                                               \
       false,                                                             \
       5,                                                                 \
       0,                                                                 \
       100000000,                                                         \
       "BMS easy operations")                                             \
  PARA(bms_random_ops,                                                    \
       int,                                                               \
       'g',                                                               \
       false,                                                             \
       250,                                                               \
       0,                                                                 \
       100000000,                                                         \
       "BMS random operations")                                           \
  PARA(tabu_base, int, 'a', false, 4, 0, 100000000, "Tabu base tenure")   \
  PARA(tabu_var,                                                          \
       int,                                                               \
       'e',                                                               \
       false,                                                             \
       7,                                                                 \
       1,                                                                 \
       100000000,                                                         \
       "Tabu tenure variation (min 1)")                                   \
  PARA(activity_period,                                                   \
       int,                                                               \
       'h',                                                               \
       false,                                                             \
       100000,                                                            \
       1,                                                                 \
       100000000,                                                         \
       "Constraint activity recompute period")                            \
  PARA(break_eq_feas,                                                     \
       int,                                                               \
       'z',                                                               \
       false,                                                             \
       0,                                                                 \
       0,                                                                 \
       1,                                                                 \
       "Break feasibility on equality constraints or not")                \
  PARA(split_eq,                                                          \
       int,                                                               \
       'j',                                                               \
       false,                                                             \
       1,                                                                 \
       0,                                                                 \
       1,                                                                 \
       "Split equalities into two inequalities")                          \
  PARA(debug, int, 'd', false, 0, 0, 1, "Debug mode or not")


//            name,   short-name, must-need, default, comments
#define STR_PARAS                                                         \
  STR_PARA(model_file, 'i', true, "", ".mps/.lp format model file path")  \
  STR_PARA(sol_path, 's', false, "", ".sol format solution path")         \
  STR_PARA(start, 'm', false, "zero", "start method: zero/random")        \
  STR_PARA(restart,                                                       \
           'y',                                                           \
           false,                                                         \
           "best",                                                        \
           "restart strategy: random/best/hybrid")                        \
  STR_PARA(                                                               \
      weight, 'w', false, "monotone", "weight method: smooth/monotone")   \
  STR_PARA(lift_scoring,                                                  \
           'f',                                                           \
           false,                                                         \
           "lift_age",                                                    \
           "feas scoring: lift_age/lift_random")                          \
  STR_PARA(neighbor_scoring,                                              \
           'n',                                                           \
           false,                                                         \
           "progress_bonus",                                              \
           "infeas scoring: progress_bonus/progress_age")                 \
  STR_PARA(param_set_file,                                                \
           'c',                                                           \
           false,                                                         \
           "",                                                            \
           "parameter configuration file (.set)")

struct Paras
{
#define PARA(N, T, S, M, D, L, H, C) T N = D;
  PARAS
#undef PARA

#define STR_PARA(N, S, M, D, C) std::string N = D;
  STR_PARAS
#undef STR_PARA

  void parse_args(int argc, char* argv[]);
  void print_change();
  void load_from_file(const std::string& file_path);

private:
  bool set_param_from_string(const std::string& name,
                             const std::string& value,
                             size_t line_no,
                             const std::string& file_path);
  void validate_required() const;
};

#define INIT_ARGS g_paras.parse_args(argc, argv);

extern Paras g_paras;

#define OPT(N) (g_paras.N)
