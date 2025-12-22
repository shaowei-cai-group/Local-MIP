
/*=====================================================================================

    Filename:     main.cpp

    Description:  Main entry point for Local-MIP solver
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../local_mip/Local_MIP.h"
#include "paras.h"
#include "solver_error.h"
#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>

std::atomic<Local_MIP*> g_solver{nullptr};

void signal_handler(int p_signal)
{
  Local_MIP* solver = g_solver.load(std::memory_order_acquire);
  if (solver != nullptr)
    solver->terminate();
}

int main(int argc, char* argv[])
{
  INIT_ARGS;
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  try
  {
    std::string model_file = OPT(model_file);
    double time_limit = OPT(time_limit);
    int bound_strengthen = OPT(bound_strengthen);
    int log_obj = OPT(log_obj);
    int restart_step = OPT(restart_step);
    std::string sol_path = OPT(sol_path);
    std::string start = OPT(start);
    std::string restart = OPT(restart);
    std::string weight = OPT(weight);
    std::string lift_scoring = OPT(lift_scoring);
    std::string neighbor_scoring = OPT(neighbor_scoring);
    int random_seed = OPT(random_seed);
    double feas_tolerance = OPT(feas_tolerance);
    double opt_tolerance = OPT(opt_tolerance);
    double zero_tolerance = OPT(zero_tolerance);
    int smooth_prob = OPT(smooth_prob);
    int bms_unsat_con = OPT(bms_unsat_con);
    int bms_unsat_ops = OPT(bms_unsat_ops);
    int bms_sat_con = OPT(bms_sat_con);
    int bms_sat_ops = OPT(bms_sat_ops);
    int bms_flip_ops = OPT(bms_flip_ops);
    int bms_easy_ops = OPT(bms_easy_ops);
    int bms_random_ops = OPT(bms_random_ops);
    int tabu_base = OPT(tabu_base);
    int tabu_variation = OPT(tabu_var);
    int activity_period = OPT(activity_period);
    int break_eq_feas = OPT(break_eq_feas);
    int split_eq = OPT(split_eq);
    std::unique_ptr<Local_MIP> solver = std::make_unique<Local_MIP>();
    g_solver.store(solver.get(), std::memory_order_release);
    solver->set_model_file(model_file);
    if (time_limit != 10.0)
      solver->set_time_limit(time_limit);
    if (bound_strengthen != 1)
      solver->set_bound_strengthen(bound_strengthen);
    if (log_obj != 1)
      solver->set_log_obj(log_obj != 0);
    if (random_seed != 0)
      solver->set_random_seed(static_cast<uint32_t>(random_seed));
    if (feas_tolerance != 1e-6)
      solver->set_feas_tolerance(feas_tolerance);
    if (opt_tolerance != 1e-4)
      solver->set_opt_tolerance(opt_tolerance);
    if (zero_tolerance != 1e-9)
      solver->set_zero_tolerance(zero_tolerance);
    if (start != "zero")
      solver->set_start_method(start);
    if (weight != "monotone")
      solver->set_weight_method(weight);
    if (lift_scoring != "lift_age")
      solver->set_lift_scoring_method(lift_scoring);
    if (neighbor_scoring != "progress_bonus")
      solver->set_neighbor_scoring_method(neighbor_scoring);
    if (restart != "best")
      solver->set_restart_method(restart);
    if (restart_step != 1000000)
      solver->set_restart_step(restart_step);
    if (smooth_prob != 1)
      solver->set_weight_smooth_probability(smooth_prob);
    if (bms_unsat_con != 12)
      solver->set_bms_unsat_con(static_cast<size_t>(bms_unsat_con));
    if (bms_unsat_ops != 2250)
      solver->set_bms_mtm_unsat_op(static_cast<size_t>(bms_unsat_ops));
    if (bms_sat_con != 1)
      solver->set_bms_sat_con(static_cast<size_t>(bms_sat_con));
    if (bms_sat_ops != 80)
      solver->set_bms_mtm_sat_op(static_cast<size_t>(bms_sat_ops));
    if (bms_flip_ops != 0)
      solver->set_bms_flip_op(static_cast<size_t>(bms_flip_ops));
    if (bms_easy_ops != 5)
      solver->set_bms_easy_op(static_cast<size_t>(bms_easy_ops));
    if (bms_random_ops != 250)
      solver->set_bms_random_op(static_cast<size_t>(bms_random_ops));
    if (tabu_base != 4)
      solver->set_tabu_base(static_cast<size_t>(tabu_base));
    if (activity_period != 100000)
      solver->set_activity_period(static_cast<size_t>(activity_period));
    if (tabu_variation != 7)
      solver->set_tabu_variation(static_cast<size_t>(tabu_variation));
    if (break_eq_feas != 0)
      solver->set_break_eq_feas(true);
    if (split_eq != 1)
      solver->set_split_eq(false);
    if (!sol_path.empty())
      solver->set_sol_path(sol_path);
    solver->run();
    g_solver.store(nullptr, std::memory_order_release);
    return 0;
  }
  catch (const Solver_Error& error)
  {
    g_solver.store(nullptr, std::memory_order_release);
    fprintf(stderr, "e %s\n", error.what());
  }
  catch (const std::exception& error)
  {
    g_solver.store(nullptr, std::memory_order_release);
    fprintf(stderr, "e unexpected error: %s\n", error.what());
  }
  catch (...)
  {
    g_solver.store(nullptr, std::memory_order_release);
    fprintf(stderr, "e unknown error.\n");
  }
  return 1;
}
