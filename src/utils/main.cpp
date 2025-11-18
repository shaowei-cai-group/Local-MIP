
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
  g_paras.print_change();
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
    solver->set_time_limit(time_limit);
    solver->set_bound_strengthen(bound_strengthen);
    solver->set_log_obj(log_obj);
    solver->set_random_seed(static_cast<uint32_t>(random_seed));
    solver->set_feas_tolerance(feas_tolerance);
    solver->set_opt_tolerance(opt_tolerance);
    solver->set_zero_tolerance(zero_tolerance);
    solver->set_start_method(start);
    solver->set_weight_method(weight);
    solver->set_lift_scoring_method(lift_scoring);
    solver->set_neighbor_scoring_method(neighbor_scoring);
    solver->set_restart_method(restart);
    solver->set_restart_step(restart_step);
    solver->set_weight_smooth_probability(smooth_prob);
    solver->set_bms_unsat_con(static_cast<size_t>(bms_unsat_con));
    solver->set_bms_mtm_unsat_op(static_cast<size_t>(bms_unsat_ops));
    solver->set_bms_sat_con(static_cast<size_t>(bms_sat_con));
    solver->set_bms_mtm_sat_op(static_cast<size_t>(bms_sat_ops));
    solver->set_bms_flip_op(static_cast<size_t>(bms_flip_ops));
    solver->set_bms_easy_op(static_cast<size_t>(bms_easy_ops));
    solver->set_bms_random_op(static_cast<size_t>(bms_random_ops));
    solver->set_tabu_base(static_cast<size_t>(tabu_base));
    solver->set_activity_period(static_cast<size_t>(activity_period));
    solver->set_tabu_variation(static_cast<size_t>(tabu_variation));
    solver->set_break_eq_feas(break_eq_feas != 0);
    solver->set_split_eq(split_eq != 0);
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
