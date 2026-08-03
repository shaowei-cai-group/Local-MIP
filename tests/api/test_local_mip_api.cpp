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
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
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
#include "reader/Model_Reader.h"
#undef private
#undef protected
#include "model_api/Model_Builder.h"
#include "utils/paras.h"
#include "utils/solver_error.h"

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

std::shared_ptr<const Prepared_Model>
prepare_model(const Model_Builder& p_builder, int p_bound_strengthen = 1)
{
  Model_Prepare_Options options;
  options.bound_strengthen = p_bound_strengthen;
  return p_builder.prepare(options);
}

bool test_constructor_defaults()
{
  Local_MIP solver;
  bool ok = true;
  ok &= check(solver.get_model_manager() != nullptr,
              "model manager should be constructed");
  ok &= check(solver.m_local_search != nullptr,
              "local search should be constructed");
  ok &= check(solver.m_model_file.empty(),
              "model file should be empty by default");
  ok &= check(solver.m_start_sol_path.empty(),
              "start solution path should be empty by default");
  ok &= check(std::fabs(solver.m_time_limit - 10.0) < 1e-9,
              "default time limit should be 10.0");
  ok &= check(solver.m_cancel_timeout,
              "timeout cancellation flag should start as true");
  ok &= check(solver.m_local_search->m_sol_path.empty(),
              "solution path should be empty by default");
  ok &= check(solver.m_log_obj_enabled,
              "log obj should be enabled by default");
  ok &= check(!solver.m_run_started,
              "solver should not be marked as run before run()");
  ok &= check(solver.get_model_manager()->m_split_eq,
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
  ok &= check(!solver.get_model_manager()->m_bound_strengthen,
              "set_bound_strengthen should forward to model manager");

  solver.set_split_eq(false);
  ok &= check(!solver.get_model_manager()->m_split_eq,
              "set_split_eq should update model manager flag");

  solver.set_log_obj(true);
  ok &= check(solver.m_log_obj_enabled,
              "set_log_obj should enable logging flag");

  const std::string sol_path = "test-output.sol";
  solver.set_sol_path(sol_path);
  ok &= check(solver.m_local_search->m_sol_path == sol_path,
              "set_sol_path should forward to local search");

  const std::string start_sol_path = "test-start.sol";
  solver.set_start_sol_path(start_sol_path);
  ok &= check(solver.m_start_sol_path == start_sol_path,
              "set_start_sol_path should store the provided path");

  solver.set_start_method("random");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::random,
              "set_start_method should recognise random");
  solver.set_start_method("objective");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::objective_guided,
              "set_start_method should recognise objective guidance");
  solver.set_start_method("locks");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::lock_guided,
              "set_start_method should recognise lock guidance");
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

bool test_model_file_reader()
{
  bool ok = true;

  Model_Manager mps_manager;
  read_model_file(TEST_MPS_PATH, mps_manager);
  ok &= check(mps_manager.process_after_read() &&
                  mps_manager.var_num() > 0,
              "model file reader should load MPS files");

  Model_Manager lp_manager;
  read_model_file(TEST_LP_PATH, lp_manager);
  ok &= check(lp_manager.process_after_read() &&
                  lp_manager.var_num() > 0,
              "model file reader should load LP files");

  const std::filesystem::path uppercase_path =
      "tmp_model_reader_uppercase.MPS";
  std::filesystem::copy_file(
      TEST_MPS_PATH,
      uppercase_path,
      std::filesystem::copy_options::overwrite_existing);
  Model_Manager uppercase_manager;
  read_model_file(uppercase_path.string(), uppercase_manager);
  std::filesystem::remove(uppercase_path);
  ok &= check(uppercase_manager.process_after_read() &&
                  uppercase_manager.var_num() > 0,
              "model file reader should accept uppercase extensions");

  bool empty_path_rejected = false;
  try
  {
    Model_Manager manager;
    read_model_file("", manager);
  }
  catch (const Solver_Error&)
  {
    empty_path_rejected = true;
  }
  ok &= check(empty_path_rejected,
              "model file reader should reject an empty path");

  bool unsupported_format_rejected = false;
  try
  {
    Model_Manager manager;
    read_model_file("instance.txt", manager);
  }
  catch (const Solver_Error&)
  {
    unsupported_format_rejected = true;
  }
  ok &= check(unsupported_format_rejected,
              "model file reader should reject unknown extensions");

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

bool test_concurrent_user_termination()
{
  Model_Builder builder;
  int x = builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
  builder.add_con(
      1.0, k_inf, std::vector<int>{x}, std::vector<double>{1.0});
  builder.add_con(
      k_neg_inf, 0.0, std::vector<int>{x}, std::vector<double>{1.0});
  Local_MIP solver(prepare_model(builder, 0));
  solver.set_time_limit(10.0);

  std::thread terminator(
      [&solver]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        solver.terminate();
      });
  const auto start = std::chrono::steady_clock::now();
  solver.run();
  terminator.join();
  const double elapsed = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start)
                             .count();

  bool ok = true;
  ok &= check(elapsed < 2.0,
              "Concurrent user termination should stop the search promptly");
  ok &= check(solver.m_user_termination_requested.load(
                  std::memory_order_relaxed),
              "User termination should be recorded atomically");
  ok &= check(solver.m_local_search->m_terminated.load(
                  std::memory_order_relaxed),
              "User termination should publish the search stop flag");
  ok &= check(!solver.m_timeout_thread.joinable() &&
                  !solver.m_obj_log_thread.joinable(),
              "Run thread should own and join background threads");
  return ok;
}

bool test_one_shot_run_guard()
{
  Local_MIP solver;
  solver.set_model_file(TEST_MPS_PATH);
  solver.set_bound_strengthen(0);
  solver.set_time_limit(5.0);
  solver.set_log_obj(false);

  std::mutex gate_mutex;
  std::condition_variable gate_cv;
  bool callback_entered = false;
  bool release_callback = false;
  solver.set_start_cbk(
      [&](Start::Start_Ctx&, void*)
      {
        std::unique_lock<std::mutex> lock(gate_mutex);
        callback_entered = true;
        gate_cv.notify_all();
        gate_cv.wait(lock, [&]() { return release_callback; });
      });

  std::exception_ptr worker_error;
  std::thread worker(
      [&]()
      {
        try
        {
          solver.run();
        }
        catch (...)
        {
          worker_error = std::current_exception();
        }
      });

  bool ok = true;
  bool entered = false;
  {
    std::unique_lock<std::mutex> lock(gate_mutex);
    entered = gate_cv.wait_for(
        lock,
        std::chrono::seconds(10),
        [&]() { return callback_entered; });
    if (!entered)
      release_callback = true;
  }
  if (!entered)
  {
    solver.terminate();
    gate_cv.notify_all();
    worker.join();
    return check(false, "Start callback should gate the active run");
  }

  bool second_run_rejected = false;
  try
  {
    solver.run();
  }
  catch (const std::logic_error& error)
  {
    second_run_rejected =
        std::string(error.what()).find("only be called once") !=
        std::string::npos;
  }
  ok &= check(second_run_rejected,
              "A second run() on one solver should be rejected");

  bool concurrent_setter_rejected = false;
  try
  {
    solver.set_random_seed(99);
  }
  catch (const std::logic_error& error)
  {
    concurrent_setter_rejected =
        std::string(error.what()).find("after run() has started") !=
        std::string::npos;
  }
  ok &= check(concurrent_setter_rejected,
              "Configuration changes during run() should be rejected");

  solver.terminate();
  {
    std::lock_guard<std::mutex> lock(gate_mutex);
    release_callback = true;
  }
  gate_cv.notify_all();
  worker.join();
  ok &= check(worker_error == nullptr,
              "The original run should complete without an exception");

  bool sequential_run_rejected = false;
  try
  {
    solver.run();
  }
  catch (const std::logic_error& error)
  {
    sequential_run_rejected =
        std::string(error.what()).find("only be called once") !=
        std::string::npos;
  }
  ok &= check(sequential_run_rejected,
              "Sequential run() reuse should be rejected");

  bool post_run_setter_rejected = false;
  try
  {
    solver.set_random_seed(99);
  }
  catch (const std::logic_error& error)
  {
    post_run_setter_rejected =
        std::string(error.what()).find("after run() has started") !=
        std::string::npos;
  }
  ok &= check(post_run_setter_rejected,
              "Configuration changes after run() should be rejected");
  return ok;
}

bool test_invalid_numeric_setters()
{
  Local_MIP solver;
  bool ok = true;
  auto rejects = [](auto&& setter)
  {
    try
    {
      setter();
    }
    catch (const std::invalid_argument&)
    {
      return true;
    }
    return false;
  };

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  ok &= check(rejects([&]() { solver.set_time_limit(nan); }),
              "NaN time limit should be rejected");
  ok &= check(rejects([&]() { solver.set_time_limit(inf); }),
              "Infinite time limit should be rejected");
  ok &= check(
      rejects([&]() { solver.set_time_limit(k_max_time_limit + 1.0); }),
      "Time limit above the documented maximum should be rejected");
  ok &= check(rejects([&]() { solver.set_feas_tolerance(-1.0); }),
              "Negative feasibility tolerance should be rejected");
  ok &= check(rejects(
                  [&]()
                  {
                    solver.set_feas_tolerance(
                        k_max_feas_tolerance * 2.0);
                  }),
              "Feasibility tolerance above its range should be rejected");
  ok &= check(rejects([&]() { solver.set_opt_tolerance(inf); }),
              "Infinite optimality tolerance should be rejected");
  ok &= check(rejects(
                  [&]()
                  {
                    solver.set_opt_tolerance(
                        k_max_opt_tolerance + 1.0);
                  }),
              "Optimality tolerance above its range should be rejected");
  ok &= check(rejects([&]() { solver.set_zero_tolerance(nan); }),
              "NaN zero tolerance should be rejected");
  ok &= check(rejects(
                  [&]()
                  {
                    solver.set_zero_tolerance(
                        k_max_zero_tolerance * 2.0);
                  }),
              "Zero tolerance above its range should be rejected");
  ok &= check(rejects([&]() { solver.set_tabu_variation(0); }),
              "Zero tabu variation should be rejected");

  const double long_long_upper_exclusive =
      -static_cast<double>(std::numeric_limits<long long>::lowest());
  ok &= check(!fits_in_long_long(long_long_upper_exclusive),
              "The first value above LLONG_MAX must not be converted");
  ok &= check(fits_in_long_long(
                  std::nextafter(long_long_upper_exclusive, 0.0)),
              "The preceding representable double should be convertible");
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
  std::fprintf(fp, "start_sol_path config-start.sol\n");
  std::fprintf(fp, "start objective\n");
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
                                       "--start_sol_path",
                                       "cli-start.sol",
                                       "--start",
                                       "locks",
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
  ok &= check(parameters.start_sol_path == std::string("cli-start.sol"),
              "CLI should override config for start_sol_path");
  ok &= check(parameters.start == std::string("locks"),
              "CLI should override config for start method");
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
  std::fprintf(fp, "start_sol_path config-start.sol\n");
  std::fprintf(fp, "start objective\n");
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
  ok &= check(solver.m_start_sol_path == std::string("config-start.sol"),
              "set_param_set_file should apply start_sol_path");
  ok &= check(solver.m_local_search->m_start.m_default_method ==
                  Start::Method::objective_guided,
              "set_param_set_file should apply objective start");
  ok &= check(solver.m_local_search->m_tabu_base == 7,
              "set_param_set_file should apply tabu_base");
  ok &= check(solver.get_model_manager()->m_split_eq,
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
  std::fprintf(fp, "time_limit = 1abc\n");
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
            "invalid floating value '1abc' for parameter 'time_limit'") !=
        std::string::npos;
  }

  ok &= check(invalid_file_threw,
              "malformed parameter files should raise an exception");
  ok &= check(std::fabs(solver.m_time_limit - 1.25) < 1e-9,
              "malformed parameter files should not partially apply values");
  ok &= check(solver.m_param_set_file.empty(),
              "malformed parameter files should not record a path");

  std::remove(invalid_config_file);

  const char* non_finite_config_file =
      "tmp_non_finite_library_params.set";
  fp = std::fopen(non_finite_config_file, "w");
  if (fp == nullptr)
  {
    std::fprintf(stderr,
                 "ERROR: failed to create non-finite parameter file\n");
    return false;
  }
  std::fprintf(fp, "time_limit = nan\n");
  std::fclose(fp);

  bool non_finite_file_threw = false;
  try
  {
    solver.set_param_set_file(non_finite_config_file);
  }
  catch (const std::exception& ex)
  {
    non_finite_file_threw =
        std::string(ex.what()).find("time_limit") != std::string::npos;
  }
  ok &= check(non_finite_file_threw,
              "Non-finite parameter-file values should be rejected");
  std::remove(non_finite_config_file);

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

bool test_integrality_guards()
{
  bool ok = true;

  {
    const double inf = std::numeric_limits<double>::infinity();
    Model_Builder builder;
    int x = builder.add_var("x", -inf, inf, 0.0, Var_Type::real);
    Local_MIP solver(prepare_model(builder));
    solver.set_log_obj(false);
    solver.run();

    const auto& model_var =
        solver.get_model_manager()->var(static_cast<size_t>(x));
    ok &= check(solver.is_feasible(),
                "Model API should accept infinite variable bounds");
    ok &= check(model_var.lower_bound() == k_neg_inf &&
                    model_var.upper_bound() == k_inf,
                "Model API should canonicalize infinite variable bounds");
  }

  {
    Model_Builder builder;
    int x = builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
    builder.add_con(k_neg_inf,
                    1.0,
                    std::vector<int>{x},
                    std::vector<double>{1.0});
    Local_MIP solver(prepare_model(builder));
    solver.set_start_cbk(
        [x](Start::Start_Ctx& ctx, void*)
        { ctx.m_var_current_value[x] = 0.5; });
    solver.set_log_obj(false);

    bool threw = false;
    try
    {
      solver.run();
    }
    catch (const std::exception& ex)
    {
      threw = std::string(ex.what()).find("initial solution violates") !=
              std::string::npos;
    }
    ok &= check(threw,
                "Fractional integer value from start callback should fail");
    ok &= check(!solver.is_feasible(),
                "Rejected start callback must not create a feasible result");
    ok &= check(solver.m_cancel_timeout &&
                    !solver.m_timeout_thread.joinable(),
                "Callback validation errors should clean up timeout thread");
    ok &= check(solver.m_run_started,
                "A failed run should still consume the one-shot solver");
    bool rerun_rejected = false;
    try
    {
      solver.run();
    }
    catch (const std::logic_error& error)
    {
      rerun_rejected =
          std::string(error.what()).find("only be called once") !=
          std::string::npos;
    }
    ok &= check(rerun_rejected,
                "A failed run should not permit solver reuse");
  }

  Model_Builder valid_builder;
  int x = valid_builder.add_var(
      "x", 0.0, 1.0, 0.0, Var_Type::binary);
  valid_builder.add_con(k_neg_inf,
                        1.0,
                        std::vector<int>{x},
                        std::vector<double>{1.0});
  Local_MIP valid_solver(prepare_model(valid_builder));
  valid_solver.set_start_cbk(
      [x](Start::Start_Ctx& ctx, void*)
      {
        ctx.m_var_current_value[x] =
            1.0 - 0.5 * k_default_feas_tolerance;
      });
  valid_solver.set_log_obj(false);
  valid_solver.run();
  ok &= check(valid_solver.is_feasible(),
              "Near-integer callback value should remain feasible");
  ok &= check(std::fabs(valid_solver.get_solution()[x] - 1.0) < 1e-12,
              "Near-integer callback value should be canonicalized");

  auto apply_validated_neighbor_move =
      [&valid_solver, x](double delta)
  {
    auto* search = valid_solver.m_local_search.get();
    search->m_var_current_value[x] = 0.0;
    search->apply_checked_move(
        static_cast<size_t>(x), delta, "selected neighbor move");
  };

  apply_validated_neighbor_move(
      1.0 - 0.5 * k_default_feas_tolerance);
  ok &= check(valid_solver.m_local_search->m_var_current_value[x] == 1.0,
              "Accepted integer move should store an exact integer value");

  bool move_threw = false;
  try
  {
    apply_validated_neighbor_move(0.5);
  }
  catch (const std::exception& ex)
  {
    move_threw =
        std::string(ex.what()).find("move violates variable domain") !=
        std::string::npos;
  }
  ok &= check(move_threw,
              "Fractional move on integer variable should be rejected");

  bool bound_move_threw = false;
  try
  {
    apply_validated_neighbor_move(2.0);
  }
  catch (const std::exception& ex)
  {
    bound_move_threw =
        std::string(ex.what()).find("move violates variable domain") !=
        std::string::npos;
  }
  ok &= check(bound_move_threw,
              "Out-of-bound moves should be rejected, not clipped");

  const bool saved_feasible =
      valid_solver.m_local_search->m_is_found_feasible;
  const bool saved_breakthrough =
      valid_solver.m_local_search->m_current_obj_breakthrough;
  const double saved_obj_constant =
      valid_solver.m_local_search->m_con_constant[0];

  valid_solver.m_local_search->m_is_found_feasible = true;
  valid_solver.m_local_search->m_con_constant[0] = -1.0;
  valid_solver.m_local_search->m_current_obj_breakthrough = true;
  valid_solver.m_local_search->refresh_activities();
  ok &= check(!valid_solver.m_local_search->m_current_obj_breakthrough,
              "Activity refresh should clear a stale breakthrough");

  valid_solver.m_local_search->m_con_constant[0] = 1.0;
  valid_solver.m_local_search->refresh_activities();
  ok &= check(valid_solver.m_local_search->m_current_obj_breakthrough,
              "Activity refresh should detect an exact breakthrough");

  valid_solver.m_local_search->m_is_found_feasible = false;
  valid_solver.m_local_search->m_con_constant[0] = k_inf;
  valid_solver.m_local_search->refresh_activities();
  ok &= check(valid_solver.m_local_search->m_current_obj_breakthrough,
              "Breakthrough state should depend only on objective values");

  valid_solver.m_local_search->m_is_found_feasible = saved_feasible;
  valid_solver.m_local_search->m_current_obj_breakthrough =
      saved_breakthrough;
  valid_solver.m_local_search->m_con_constant[0] = saved_obj_constant;

  bool selected_index_threw = false;
  try
  {
    valid_solver.m_local_search->apply_checked_move(
        valid_solver.m_local_search->m_var_num,
        1.0,
        "selected neighbor move");
  }
  catch (const std::exception& ex)
  {
    selected_index_threw =
        std::string(ex.what()).find("variable index is out of range") !=
        std::string::npos;
  }
  ok &= check(selected_index_threw,
              "Invalid selected index should fail before applying a move");

  std::vector<Neighbor> invalid_custom_neighbors;
  invalid_custom_neighbors.emplace_back(
      "invalid",
      [](Neighbor::Neighbor_Ctx& ctx, void*)
      {
        ctx.set_single_op(ctx.m_shared.m_model_manager.var_num(), 1.0);
      });
  bool custom_neighbor_threw = false;
  try
  {
    valid_solver.m_local_search->explore_neighbor(
        invalid_custom_neighbors);
  }
  catch (const std::exception& ex)
  {
    custom_neighbor_threw =
        std::string(ex.what()).find("variable index is out of range") !=
        std::string::npos;
  }
  ok &= check(custom_neighbor_threw,
              "Custom-neighbor indices should be safe before scoring");

  valid_solver.m_local_search->m_var_current_value[x] = 0.0;
  valid_solver.m_local_search->m_scoring.set_neighbor_cbk(
      [x](Scoring::Neighbor_Ctx& ctx,
          size_t var_idx,
          double delta,
          void*)
      {
        if (var_idx == static_cast<size_t>(x) && delta == 1.0)
        {
          ctx.m_best_var_idx = var_idx;
          ctx.m_best_delta = delta;
          ctx.m_best_neighbor_score = 1;
        }
      });
  std::vector<Neighbor> mixed_custom_neighbors;
  mixed_custom_neighbors.emplace_back(
      "mixed",
      [x](Neighbor::Neighbor_Ctx& ctx, void*)
      {
        ctx.append_op(static_cast<size_t>(x), 0.5);
        ctx.append_op(static_cast<size_t>(x), 1.0);
      });
  bool validate_selected =
      valid_solver.m_local_search->explore_neighbor(mixed_custom_neighbors);
  ok &= check(validate_selected,
              "User callbacks should request one final selected-move check");
  valid_solver.m_local_search->apply_checked_move(
      valid_solver.m_local_search->m_best_var_idx,
      valid_solver.m_local_search->m_best_delta,
      "selected neighbor move");
  ok &= check(valid_solver.m_local_search->m_var_current_value[x] == 1.0,
              "Only the selected callback move should be normalized");

  valid_solver.m_local_search->m_scoring.set_neighbor_cbk(
      [](Scoring::Neighbor_Ctx& ctx, size_t, double, void*)
      {
        ctx.m_best_var_idx = ctx.m_shared.m_model_manager.var_num();
        ctx.m_best_delta = 1.0;
        ctx.m_best_neighbor_score = 1;
      });
  std::vector<Neighbor> valid_custom_neighbor;
  valid_custom_neighbor.emplace_back(
      "valid",
      [x](Neighbor::Neighbor_Ctx& ctx, void*)
      { ctx.set_single_op(static_cast<size_t>(x), -1.0); });
  validate_selected =
      valid_solver.m_local_search->explore_neighbor(valid_custom_neighbor);
  bool scoring_selection_threw = false;
  try
  {
    valid_solver.m_local_search->apply_checked_move(
        valid_solver.m_local_search->m_best_var_idx,
        valid_solver.m_local_search->m_best_delta,
        "selected neighbor move");
  }
  catch (const std::exception& ex)
  {
    scoring_selection_threw =
        std::string(ex.what()).find("variable index is out of range") !=
        std::string::npos;
  }
  ok &= check(validate_selected && scoring_selection_threw,
              "Invalid scoring callback selection should fail only at apply");

  {
    Model_Builder builder;
    int lift_x = builder.add_var(
        "x", 0.0, 1.0, 1.0, Var_Type::binary);
    builder.add_con(k_neg_inf,
                    1.0,
                    std::vector<int>{lift_x},
                    std::vector<double>{1.0});
    Local_MIP lift_solver(prepare_model(builder));
    lift_solver.set_time_limit(0.05);
    lift_solver.set_log_obj(false);
    lift_solver.run();

    auto* search = lift_solver.m_local_search.get();
    search->m_scoring.set_lift_cbk(
        [](Scoring::Lift_Ctx& ctx, size_t, double, void*)
        {
          ctx.m_best_var_idx = ctx.m_shared.m_model_manager.var_num();
          ctx.m_best_delta = 1.0;
          ctx.m_best_lift_score = 1.0;
        });
    bool lift_selection_threw = false;
    try
    {
      search->lift_move();
    }
    catch (const std::exception& ex)
    {
      lift_selection_threw =
          std::string(ex.what()).find("variable index is out of range") !=
          std::string::npos;
    }
    ok &= check(lift_selection_threw,
                "Invalid lift-scoring selection should fail at apply");
  }

  {
    const double lower = 1e19;
    const double upper = lower + 1e6;
    Model_Builder builder;
    int huge_x = builder.add_var(
        "huge", lower, upper, 0.0, Var_Type::general_integer);
    builder.add_con(k_neg_inf,
                    upper,
                    std::vector<int>{huge_x},
                    std::vector<double>{1.0});
    Local_MIP huge_integer_solver(prepare_model(builder, 0));
    huge_integer_solver.set_start_method("random");
    huge_integer_solver.set_log_obj(false);
    huge_integer_solver.run();

    auto* search = huge_integer_solver.m_local_search.get();
    const auto& model_var =
        huge_integer_solver.get_model_manager()->var(huge_x);
    ok &= check(!fits_in_long_long(lower) && !fits_in_long_long(upper),
                "Huge integer bounds should not be cast to long long");
    ok &= check(huge_integer_solver.get_model_manager()->var_in_bound(
                    model_var, search->m_var_current_value[huge_x]),
                "Random start should keep huge integers in their domain");
    search->m_is_found_feasible = false;
    const double restart_value =
        search->m_restart.sample_random_value(search->m_restart_ctx,
                                              model_var);
    ok &= check(std::isfinite(restart_value) &&
                    huge_integer_solver.get_model_manager()->var_in_bound(
                        model_var, restart_value),
                "Restart should safely handle integer bounds above LLONG_MAX");
  }

  const char* invalid_output_file = "tmp_invalid_integer_output.sol";
  std::remove(invalid_output_file);
  valid_solver.m_local_search->m_sol_path = invalid_output_file;
  valid_solver.m_local_search->m_var_best_value[x] = 0.5;
  valid_solver.m_local_search->m_is_found_feasible = true;
  valid_solver.m_local_search->write_sol();
  ok &= check(!valid_solver.m_local_search->finalize_result(),
              "Invalid integer result should fail final validation");
  ok &= check(!valid_solver.is_feasible(),
              "Failed final integrality verification should clear status");
  std::FILE* invalid_output = std::fopen(invalid_output_file, "r");
  ok &= check(invalid_output == nullptr,
              "Invalid integer solution must not be written");
  if (invalid_output != nullptr)
  {
    std::fclose(invalid_output);
    std::remove(invalid_output_file);
  }

  {
    Model_Builder builder;
    int restart_x = builder.add_var(
        "x", 0.0, 1.0, 0.0, Var_Type::binary);
    builder.add_con(1.0,
                    k_inf,
                    std::vector<int>{restart_x},
                    std::vector<double>{1.0});
    builder.add_con(k_neg_inf,
                    0.0,
                    std::vector<int>{restart_x},
                    std::vector<double>{1.0});
    Local_MIP restart_solver(prepare_model(builder, 0));
    restart_solver.set_restart_step(1);
    restart_solver.set_restart_cbk(
        [restart_x](Restart::Restart_Ctx& ctx, void*)
        { ctx.m_var_current_value[restart_x] = 0.5; });
    restart_solver.set_time_limit(0.2);
    restart_solver.set_log_obj(false);

    bool restart_threw = false;
    try
    {
      restart_solver.run();
    }
    catch (const std::exception& ex)
    {
      restart_threw =
          std::string(ex.what()).find("restart solution violates") !=
          std::string::npos;
    }
    ok &= check(restart_threw,
                "Fractional integer value from restart callback should fail");
  }

  const char* objective_only_file =
      "tmp_objective_only_integer_bounds.lp";
  std::FILE* fp = std::fopen(objective_only_file, "w");
  if (fp == nullptr)
    return check(false, "Should create objective-only LP file");
  std::fprintf(fp,
               "Minimize\n obj: x\nBounds\n 0.5 <= x <= 2.5\n"
               "General\n x\nEnd\n");
  std::fclose(fp);

  Local_MIP objective_only_solver;
  objective_only_solver.set_model_file(objective_only_file);
  objective_only_solver.set_log_obj(false);
  objective_only_solver.run();
  ok &= check(objective_only_solver.is_feasible(),
              "Objective-only integer model should be feasible");
  ok &= check(
      std::fabs(objective_only_solver.get_solution()[0] - 1.0) < 1e-12,
      "Objective-only shortcut should choose an integer bound");
  std::remove(objective_only_file);

  const char* partial_file = "tmp_partial_domain_start.sol";
  fp = std::fopen(partial_file, "w");
  if (fp == nullptr)
    return check(false, "Should create partial warm-start file");
  std::fprintf(fp, "Variable name        Variable value\n");
  std::fprintf(fp, "x 0\n");
  std::fclose(fp);

  Model_Builder partial_builder;
  int partial_x = partial_builder.add_var(
      "x", 0.0, 1.0, 0.0, Var_Type::binary);
  int y = partial_builder.add_var(
      "y", 2.0, 3.0, 0.0, Var_Type::general_integer);
  partial_builder.add_con(k_neg_inf,
                          1.0,
                          std::vector<int>{partial_x},
                          std::vector<double>{1.0});
  partial_builder.add_con(k_neg_inf,
                          3.0,
                          std::vector<int>{y},
                          std::vector<double>{1.0});
  Local_MIP partial_solver(prepare_model(partial_builder));
  partial_solver.set_start_sol_path(partial_file);
  partial_solver.set_log_obj(false);
  partial_solver.run();
  ok &= check(partial_solver.is_feasible(),
              "Partial warm start should produce a valid solution");
  ok &= check(std::fabs(partial_solver.get_solution()[y] - 2.0) < 1e-12,
              "Missing warm-start value should use domain-valid zero start");
  std::remove(partial_file);

  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_constructor_defaults();
  ok &= test_basic_setters();
  ok &= test_model_file_reader();
  ok &= test_run_with_timeout();
  ok &= test_concurrent_user_termination();
  ok &= test_one_shot_run_guard();
  ok &= test_invalid_numeric_setters();
  ok &= test_parameter_file_loading();
  ok &= test_library_parameter_file_loading();
  ok &= test_library_parameter_file_errors();
  ok &= test_integrality_guards();

  if (!ok)
  {
    std::fprintf(stderr, "c Local_MIP API tests FAILED.\n");
    return EXIT_FAILURE;
  }

  std::printf("c Local_MIP API tests PASSED.\n");
  return EXIT_SUCCESS;
}
