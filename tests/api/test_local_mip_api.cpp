/*=====================================================================================

    Filename:     test_local_mip_api.cpp

    Description:  Local-MIP API Tests - Tests solver public interfaces and basic functionality
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>  // Include standard library headers BEFORE macros

#define private public
#define protected public
#include "Local_MIP.h"
#include "local_search/Local_Search.h"
#include "local_search/restart/restart.h"
#include "local_search/scoring/scoring.h"
#include "local_search/start/start.h"
#include "local_search/weight/weight.h"
#include "reader/LP_Reader.h"
#include "reader/MPS_Reader.h"
#undef private
#undef protected
#include "utils/paras.h"

namespace
{

bool check(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "ERROR: %s\n", message);
    return false;
  }
  return true;
}

bool test_constructor_defaults()
{
  Local_MIP solver;
  bool ok = true;
  ok &= check(solver.m_model_manager != nullptr,
              "model manager should be constructed");
  ok &= check(solver.m_local_search != nullptr,
              "local search should be constructed");
  ok &= check(solver.m_reader == nullptr,
              "reader should be null before prepare_reader()");
  ok &= check(solver.m_model_file.empty(),
              "model file should be empty by default");
  ok &= check(std::fabs(solver.m_time_limit - 10.0) < 1e-9,
              "default time limit should be 10.0");
  ok &= check(solver.m_cancel_timeout,
              "timeout cancellation flag should start as true");
  ok &= check(solver.m_local_search->m_sol_path.empty(),
              "solution path should be empty by default");
  ok &= check(solver.m_log_obj_enabled,
              "log obj should be enabled by default");
  ok &= check(solver.m_model_manager->m_split_eq,
              "split equality conversion should be enabled by default");
  return ok;
}

bool test_basic_setters()
{
  Local_MIP solver;
  bool ok = true;

  solver.set_model_file(TEST_MPS_PATH);
  ok &= check(solver.m_model_file == std::string(TEST_MPS_PATH),
              "set_model_file should update internal path");

  solver.set_time_limit(4.5);
  ok &= check(std::fabs(solver.m_time_limit - 4.5) < 1e-9,
              "set_time_limit should store the provided value");

  solver.set_bound_strengthen(false);
  ok &= check(!solver.m_model_manager->m_bound_strengthen,
              "set_bound_strengthen should forward to model manager");

  solver.set_split_eq(false);
  ok &= check(!solver.m_model_manager->m_split_eq,
              "set_split_eq should update model manager flag");

  solver.set_log_obj(true);
  ok &= check(solver.m_log_obj_enabled,
              "set_log_obj should enable logging flag");

  const std::string sol_path = "test-output.sol";
  solver.set_sol_path(sol_path);
  ok &= check(solver.m_local_search->m_sol_path == sol_path,
              "set_sol_path should forward to local search");

  solver.set_start_method("random");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::random,
              "set_start_method should recognise random");
  solver.set_start_method("unsupported-method");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::zero,
              "set_start_method should fallback to zero");

  solver.set_restart_method("hybrid");
  ok &= check(solver.m_local_search->m_restart.m_default_strategy ==
                  Restart::Strategy::hybrid,
              "set_restart_method should recognise hybrid");
  solver.set_restart_method("unknown");
  ok &= check(solver.m_local_search->m_restart.m_default_strategy ==
                  Restart::Strategy::random,
              "set_restart_method should fallback to random");

  solver.set_restart_step(32);
  ok &= check(solver.m_local_search->m_restart.m_restart_step == 32,
              "set_restart_step should propagate to restart");

  solver.set_weight_method("monotone");
  ok &= check(solver.m_local_search->m_weight.m_default_method ==
                  Weight::Method::monotone,
              "set_weight_method should recognise monotone");
  solver.set_weight_method("DEFAULT");
  ok &= check(solver.m_local_search->m_weight.m_default_method ==
                  Weight::Method::smooth,
              "set_weight_method should fallback to smooth");

  solver.set_lift_scoring_method("lift_random");
  ok &= check(solver.m_local_search->m_scoring.m_lift_method ==
                  Scoring::Lift_Method::lift_random,
              "set_lift_scoring_method should recognise lift_random");
  solver.set_lift_scoring_method("unsupported");
  ok &= check(solver.m_local_search->m_scoring.m_lift_method ==
                  Scoring::Lift_Method::lift_age,
              "set_lift_scoring_method should fallback to default");

  solver.set_neighbor_scoring_method("progress_age");
  ok &= check(solver.m_local_search->m_scoring.m_neighbor_method ==
                  Scoring::Neighbor_Method::progress_age,
              "set_neighbor_scoring_method should recognise progress_age");
  solver.set_neighbor_scoring_method("UNKNOWN");
  ok &= check(
      solver.m_local_search->m_scoring.m_neighbor_method ==
          Scoring::Neighbor_Method::progress_bonus,
      "set_neighbor_scoring_method should fallback to progress_bonus");

  solver.set_weight_smooth_probability(17);
  ok &= check(solver.m_local_search->m_weight.smooth_probability() == 17,
              "set_weight_smooth_probability should persist value");

  solver.set_bms_unsat_con(25);
  ok &= check(solver.m_local_search->m_bms_unsat_con == 25,
              "set_bms_unsat_con should update local search");

  solver.set_bms_mtm_unsat_op(1234);
  ok &= check(solver.m_local_search->m_bms_mtm_unsat_op == 1234,
              "set_bms_mtm_unsat_op should update local search");

  solver.set_bms_sat_con(40);
  ok &= check(solver.m_local_search->m_bms_sat_con == 40,
              "set_bms_sat_con should update local search");

  solver.set_bms_mtm_sat_op(250);
  ok &= check(solver.m_local_search->m_bms_mtm_sat_op == 250,
              "set_bms_mtm_sat_op should update local search");

  solver.set_bms_flip_op(35);
  ok &= check(solver.m_local_search->m_bms_flip_op == 35,
              "set_bms_flip_op should update local search");

  solver.set_bms_random_op(210);
  ok &= check(solver.m_local_search->m_bms_random_op == 210,
              "set_bms_random_op should update local search");

  solver.set_tabu_base(6);
  ok &= check(solver.m_local_search->m_tabu_base == 6,
              "set_tabu_base should update local search");

  solver.set_tabu_variation(14);
  ok &= check(solver.m_local_search->m_tabu_variation == 14,
              "set_tabu_variation should update local search");

  solver.m_cancel_timeout = false;
  solver.terminate();
  ok &= check(solver.m_cancel_timeout,
              "terminate should request timeout thread to stop");
  ok &= check(solver.m_local_search->m_terminated,
              "terminate should set local search terminate flag");

  return ok;
}

bool test_prepare_reader_selection()
{
  Local_MIP solver;
  bool ok = true;

  solver.set_model_file(TEST_MPS_PATH);
  solver.prepare_reader();
  ok &= check(solver.m_reader != nullptr,
              "prepare_reader should allocate a reader");
  ok &= check(dynamic_cast<MPS_Reader*>(solver.m_reader.get()) != nullptr,
              "prepare_reader should pick MPS reader for .mps files");

  solver.set_model_file(TEST_LP_PATH);
  solver.prepare_reader();
  ok &= check(dynamic_cast<LP_Reader*>(solver.m_reader.get()) != nullptr,
              "prepare_reader should pick LP reader for .lp files");

  return ok;
}

bool test_run_with_timeout()
{
  Local_MIP solver;
  solver.set_model_file(TEST_MPS_PATH);
  solver.set_time_limit(0.1);
  solver.set_bound_strengthen(false);
  solver.set_log_obj(false);

  const auto start = std::chrono::steady_clock::now();
  solver.run();
  const double elapsed = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start)
                             .count();

  bool ok = true;
  ok &= check(elapsed < 5.0,
              "Local_MIP::run should finish within the timeout window");
  ok &= check(solver.m_cancel_timeout,
              "run should cancel the timeout thread");
  ok &= check(!solver.m_timeout_thread.joinable(),
              "timeout thread should be joined after run");
  ok &= check(solver.m_local_search->m_terminated,
              "local search should terminate by timeout thread");

  return ok;
}

bool test_parameter_file_loading()
{
  const char* config_file = "tmp_params.set";
  std::FILE* fp = std::fopen(config_file, "w");
  if (fp == nullptr)
  {
    std::fprintf(stderr,
                 "ERROR: failed to create temporary config file\n");
    return false;
  }

  std::fprintf(fp, "# sample parameter file\n");
  std::fprintf(fp, "time_limit = 33\n");
  std::fprintf(fp, "model_file %s\n", TEST_MPS_PATH);
  std::fprintf(fp, "bms_unsat_con 88\n");
  std::fprintf(fp, "bms_random_ops = 199\n");
  std::fprintf(fp, "sol_path config.sol\n");
  std::fprintf(fp, "tabu_base 7\n");
  std::fprintf(fp, "split_eq 1\n");
  std::fprintf(fp, "c comment line should be ignored\n");
  std::fclose(fp);

  Paras parameters;
  std::vector<std::string> cli_args = {"solver",
                                       "--param_set_file",
                                       config_file,
                                       "--bms_unsat_con",
                                       "55",
                                       "--sol_path",
                                       "cli.sol",
                                       "--split_eq",
                                       "0"};

  std::vector<std::unique_ptr<char[]>> storage(cli_args.size());
  std::vector<char*> argv(cli_args.size());
  for (size_t idx = 0; idx < cli_args.size(); ++idx)
  {
    storage[idx] = std::make_unique<char[]>(cli_args[idx].size() + 1);
    std::strcpy(storage[idx].get(), cli_args[idx].c_str());
    argv[idx] = storage[idx].get();
  }

  parameters.parse_args(static_cast<int>(argv.size()), argv.data());

  bool ok = true;
  ok &= check(std::fabs(parameters.time_limit - 33.0) < 1e-9,
              "config file should set time_limit");
  ok &= check(parameters.model_file == std::string(TEST_MPS_PATH),
              "config file should set required model_file");
  ok &= check(parameters.bms_unsat_con == 55,
              "CLI should override config for bms_unsat_con");
  ok &= check(parameters.bms_random_ops == 199,
              "config file should set bms_random_ops");
  ok &= check(parameters.sol_path == std::string("cli.sol"),
              "CLI should override config for sol_path");
  ok &= check(parameters.split_eq == 0,
              "CLI should override config for split_eq");
  ok &=
      check(parameters.tabu_base == 7, "config file should set tabu_base");
  ok &= check(parameters.param_set_file == std::string(config_file),
              "param_set_file should record the loaded config path");

  std::remove(config_file);
  return ok;
}

bool test_library_parameter_file_loading()
{
  const char* config_file = "tmp_library_params.set";
  std::FILE* fp = std::fopen(config_file, "w");
  if (fp == nullptr)
  {
    std::fprintf(stderr,
                 "ERROR: failed to create temporary library config file\n");
    return false;
  }

  std::fprintf(fp, "time_limit = 33\n");
  std::fprintf(fp, "model_file %s\n", TEST_MPS_PATH);
  std::fprintf(fp, "bms_unsat_con 88\n");
  std::fprintf(fp, "bms_random_ops = 199\n");
  std::fprintf(fp, "sol_path config.sol\n");
  std::fprintf(fp, "tabu_base 7\n");
  std::fprintf(fp, "split_eq 1\n");
  std::fclose(fp);

  Local_MIP solver;
  solver.set_bms_sat_con(41);
  solver.set_model_file(TEST_LP_PATH);
  solver.set_param_set_file(config_file);

  bool ok = true;
  ok &= check(solver.m_param_set_file == std::string(config_file),
              "set_param_set_file should record the loaded path");
  ok &= check(std::fabs(solver.m_time_limit - 33.0) < 1e-9,
              "set_param_set_file should apply time_limit");
  ok &= check(solver.m_model_file == std::string(TEST_MPS_PATH),
              "set_param_set_file should apply model_file");
  ok &= check(solver.m_local_search->m_bms_unsat_con == 88,
              "set_param_set_file should apply bms_unsat_con");
  ok &= check(solver.m_local_search->m_bms_random_op == 199,
              "set_param_set_file should apply bms_random_ops");
  ok &= check(solver.m_local_search->m_sol_path == std::string("config.sol"),
              "set_param_set_file should apply sol_path");
  ok &= check(solver.m_local_search->m_tabu_base == 7,
              "set_param_set_file should apply tabu_base");
  ok &= check(solver.m_model_manager->m_split_eq,
              "set_param_set_file should apply split_eq");
  ok &= check(solver.m_local_search->m_bms_sat_con == 41,
              "set_param_set_file should not reset unspecified values");

  solver.set_time_limit(4.5);
  ok &= check(std::fabs(solver.m_time_limit - 4.5) < 1e-9,
              "later setters should override file-loaded values");

  std::remove(config_file);
  return ok;
}

bool test_library_parameter_file_errors()
{
  Local_MIP solver;
  solver.set_time_limit(1.25);

  bool ok = true;
  bool missing_file_threw = false;
  try
  {
    solver.set_param_set_file("tmp_missing_library_params.set");
  }
  catch (const std::exception& ex)
  {
    missing_file_threw =
        std::string(ex.what()).find("cannot open parameter set file") !=
        std::string::npos;
  }

  ok &= check(missing_file_threw,
              "missing parameter file should raise an exception");
  ok &= check(std::fabs(solver.m_time_limit - 1.25) < 1e-9,
              "failed parameter-file loads should not mutate solver state");
  ok &= check(solver.m_param_set_file.empty(),
              "failed parameter-file loads should not record a path");

  const char* invalid_config_file = "tmp_invalid_library_params.set";
  std::FILE* fp = std::fopen(invalid_config_file, "w");
  if (fp == nullptr)
  {
    std::fprintf(stderr,
                 "ERROR: failed to create invalid temporary config file\n");
    return false;
  }
  std::fprintf(fp, "time_limit = nope\n");
  std::fclose(fp);

  bool invalid_file_threw = false;
  try
  {
    solver.set_param_set_file(invalid_config_file);
  }
  catch (const std::exception& ex)
  {
    invalid_file_threw =
        std::string(ex.what()).find(
            "invalid floating value 'nope' for parameter 'time_limit'") !=
        std::string::npos;
  }

  ok &= check(invalid_file_threw,
              "malformed parameter files should raise an exception");
  ok &= check(std::fabs(solver.m_time_limit - 1.25) < 1e-9,
              "malformed parameter files should not partially apply values");
  ok &= check(solver.m_param_set_file.empty(),
              "malformed parameter files should not record a path");

  std::remove(invalid_config_file);

  const char* rejected_config_file = "tmp_rejected_library_params.set";
  fp = std::fopen(rejected_config_file, "w");
  if (fp == nullptr)
  {
    std::fprintf(stderr,
                 "ERROR: failed to create rejected temporary config file\n");
    return false;
  }
  std::fprintf(fp, "model_file %s\n", TEST_MPS_PATH);
  std::fprintf(fp, "time_limit = 0\n");
  std::fclose(fp);

  solver.set_model_file(TEST_LP_PATH);
  bool rejected_file_threw = false;
  try
  {
    solver.set_param_set_file(rejected_config_file);
  }
  catch (const std::exception& ex)
  {
    rejected_file_threw =
        std::string(ex.what()).find("time limit must be positive") !=
        std::string::npos;
  }

  ok &= check(rejected_file_threw,
              "setter-side validation failures should raise an exception");
  ok &= check(std::fabs(solver.m_time_limit - 1.25) < 1e-9,
              "setter-side failures should not partially apply values");
  ok &= check(solver.m_model_file == std::string(TEST_LP_PATH),
              "setter-side failures should leave earlier fields unchanged");
  ok &= check(solver.m_param_set_file.empty(),
              "setter-side failures should not record a path");

  std::remove(rejected_config_file);
  return ok;
}

bool test_embedded_exchange_hooks()
{
  Local_MIP solver;
  bool ok = true;

  solver.enable_model_api();
  solver.set_log_obj(false);
  solver.set_time_limit(0.05);
  solver.set_bound_strengthen(false);
  int x = solver.add_var("x", 0.0, 1.0, 1.0, Var_Type::binary);
  solver.add_con(1.0, 1.0, std::vector<int>{x}, std::vector<double>{1.0});

  size_t improvement_calls = 0;
  size_t improvement_n = 0;
  double improvement_obj = 0.0;
  solver.set_on_improvement_callback(
      [&](const double*, size_t n, double obj)
      {
        ++improvement_calls;
        improvement_n = n;
        improvement_obj = obj;
      });
  solver.run();
  ok &= check(improvement_calls > 0,
              "improvement callback should run during solve");
  ok &= check(improvement_n == 1,
              "improvement callback should report variable count");
  ok &= check(std::fabs(improvement_obj - solver.get_obj_value()) < 1e-9,
              "improvement callback should report user objective");

  Local_MIP inject_solver;
  inject_solver.enable_model_api();
  int y = inject_solver.add_var("y", 0.0, 10.0, 1.0, Var_Type::real);
  (void)y;
  inject_solver.add_con(0.0, 10.0, std::vector<int>{0}, std::vector<double>{1.0});
  inject_solver.m_model_api->build_model(*inject_solver.m_model_manager);
  inject_solver.m_model_manager->process_after_read();
  inject_solver.m_local_search->init_data();
  inject_solver.m_local_search->m_var_best_value.assign(1, 5.0);
  inject_solver.m_local_search->m_var_current_value.assign(1, 5.0);
  inject_solver.m_local_search->m_var_num = 1;
  inject_solver.m_local_search->m_best_obj = 5.0;

  double better_sol[1] = {2.0};
  ok &= check(inject_solver.inject_solution(better_sol, 1, 2.0),
              "inject_solution should accept better external objective");
  ok &= check(inject_solver.is_feasible(),
              "inject_solution should mark solver feasible");
  ok &= check(std::fabs(inject_solver.get_solution()[0] - 2.0) < 1e-9,
              "inject_solution should copy best solution");
  ok &= check(!inject_solver.inject_solution(better_sol, 2, 1.0),
              "inject_solution should reject mismatched variable count");
  ok &= check(!inject_solver.inject_solution(better_sol, 1, 9.0),
              "inject_solution should reject worse objective");

  double current_sol[1] = {3.0};
  ok &= check(inject_solver.inject_to_current_and_restart(current_sol, 1, 77),
              "inject_to_current_and_restart should accept matching vector");
  ok &= check(
      std::fabs(inject_solver.m_local_search->m_var_current_value[0] - 3.0) <
          1e-9,
      "inject_to_current_and_restart should copy current solution");
  ok &= check(inject_solver.m_local_search->m_restart.m_restart_step == 77,
              "inject_to_current_and_restart should override restart step");

  bool exchange_called = false;
  inject_solver.set_exchange_check_interval(1);
  inject_solver.set_exchange_check_callback([&]() { exchange_called = true; });
  inject_solver.m_local_search->m_exchange_check_cbk();
  ok &= check(exchange_called,
              "exchange callback should be stored and callable");

  Local_MIP infeas_solver;
  infeas_solver.enable_model_api();
  int z = infeas_solver.add_var("z", 0.0, 1.0, 0.0, Var_Type::binary);
  (void)z;
  infeas_solver.add_con(1.0, 1.0, std::vector<int>{0}, std::vector<double>{1.0});
  infeas_solver.m_model_api->build_model(*infeas_solver.m_model_manager);
  infeas_solver.m_model_manager->process_after_read();
  infeas_solver.m_local_search->init_data();
  infeas_solver.m_local_search->m_var_num = 1;
  infeas_solver.m_local_search->m_min_unsat_con = 5;

  size_t infeas_calls = 0;
  size_t infeas_n = 0;
  size_t infeas_unsat = 0;
  infeas_solver.set_on_infeas_improvement_callback(
      [&](const double*, size_t n, size_t unsat)
      {
        ++infeas_calls;
        infeas_n = n;
        infeas_unsat = unsat;
      });
  infeas_solver.m_local_search->m_on_infeas_improvement_cbk(
      nullptr, 1, 4);
  ok &= check(infeas_calls == 1 && infeas_n == 1 && infeas_unsat == 4,
              "infeas improvement callback should be stored and callable");

  ok &= check(infeas_solver.inject_infeas_solution(nullptr, 1, 3),
              "inject_infeas_solution should accept lower unsat count");
  ok &= check(infeas_solver.m_local_search->get_min_unsat_con() == 3,
              "inject_infeas_solution should update min unsat count");
  ok &= check(!infeas_solver.inject_infeas_solution(nullptr, 1, 3),
              "inject_infeas_solution should reject non-improving count");
  ok &= check(!infeas_solver.inject_infeas_solution(nullptr, 2, 2),
              "inject_infeas_solution should reject mismatched variable count");

  return ok;
}

bool test_model_api_empty_constraints()
{
  bool ok = true;

  Local_MIP sat_solver;
  sat_solver.enable_model_api();
  sat_solver.set_bound_strengthen(false);
  sat_solver.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
  sat_solver.add_con(-1.0, 2.0, std::vector<int>{}, std::vector<double>{});
  sat_solver.m_model_api->build_model(*sat_solver.m_model_manager);
  ok &= check(sat_solver.m_model_manager->process_after_read(),
              "satisfied empty constraints should be accepted");

  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_constructor_defaults();
  ok &= test_basic_setters();
  ok &= test_prepare_reader_selection();
  ok &= test_run_with_timeout();
  ok &= test_parameter_file_loading();
  ok &= test_library_parameter_file_loading();
  ok &= test_library_parameter_file_errors();
  ok &= test_embedded_exchange_hooks();
  ok &= test_model_api_empty_constraints();

  if (!ok)
  {
    std::fprintf(stderr, "c Local_MIP API tests FAILED.\n");
    return EXIT_FAILURE;
  }

  std::printf("c Local_MIP API tests PASSED.\n");
  return EXIT_SUCCESS;
}
