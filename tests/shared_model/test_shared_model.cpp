/*=====================================================================================

    Filename:     test_shared_model.cpp

    Description:  Shared Prepared_Model concurrency and ownership tests
        Version:  2.0

=====================================================================================*/

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#define private public
#define protected public
#include "Local_MIP.h"
#include "model_api/Model_Builder.h"
#undef private
#undef protected

#ifndef TEST_MPS_PATH
#define TEST_MPS_PATH "test-set/sct1.mps"
#endif

#ifndef TEST_LP_PATH
#define TEST_LP_PATH "test-set/sct1.lp"
#endif

namespace
{

bool check(bool p_condition, const char* p_message)
{
  if (!p_condition)
  {
    std::fprintf(stderr, "ERROR: %s\n", p_message);
    return false;
  }
  return true;
}

class Structure_Hash
{
private:
  std::uint64_t m_value = 14695981039346656037ULL;

public:
  void add_bytes(const void* p_data, std::size_t p_size)
  {
    const auto* bytes = static_cast<const unsigned char*>(p_data);
    for (std::size_t idx = 0; idx < p_size; ++idx)
    {
      m_value ^= bytes[idx];
      m_value *= 1099511628211ULL;
    }
  }

  template <typename T>
  void add(const T& p_value)
  {
    static_assert(std::is_trivially_copyable_v<T>);
    add_bytes(&p_value, sizeof(T));
  }

  void add_double(double p_value)
  {
    add(std::bit_cast<std::uint64_t>(p_value));
  }

  void add_string(const std::string& p_value)
  {
    add(p_value.size());
    add_bytes(p_value.data(), p_value.size());
  }

  std::uint64_t value() const
  {
    return m_value;
  }
};

std::uint64_t structure_hash(const Model_Manager& p_manager)
{
  Structure_Hash hash;
  hash.add(p_manager.var_num());
  hash.add(p_manager.con_num());
  hash.add(p_manager.is_min());
  hash.add_double(p_manager.obj_offset());
  hash.add_string(p_manager.get_obj_name());
  hash.add_double(p_manager.feas_tolerance());
  hash.add_double(p_manager.zero_tolerance());

  for (std::size_t var_idx = 0; var_idx < p_manager.var_num(); ++var_idx)
  {
    const Model_Var& var = p_manager.var(var_idx);
    hash.add_string(var.name());
    hash.add(var.idx());
    hash.add(var.type());
    hash.add(var.requires_integrality());
    hash.add_double(var.lower_bound());
    hash.add_double(var.upper_bound());
    hash.add(var.term_num());
    for (std::size_t term_idx = 0; term_idx < var.term_num(); ++term_idx)
    {
      hash.add(var.con_idx(term_idx));
      hash.add(var.pos_in_con(term_idx));
    }
  }

  for (std::size_t con_idx = 0; con_idx < p_manager.con_num(); ++con_idx)
  {
    const Model_Con& con = p_manager.con(con_idx);
    hash.add_string(con.name());
    hash.add(con.idx());
    hash.add(con.is_equality());
    hash.add(con.is_greater());
    hash.add(con.is_inferred_sat());
    hash.add_double(con.rhs());
    hash.add(con.term_num());
    for (std::size_t term_idx = 0; term_idx < con.term_num(); ++term_idx)
    {
      hash.add(con.var_idx(term_idx));
      hash.add_double(con.coeff(term_idx));
    }
    hash.add(con.get_types().size());
    for (Con_Type type : con.get_types())
      hash.add(type);
  }

  hash.add(p_manager.binary_idx_list().size());
  for (std::size_t var_idx : p_manager.binary_idx_list())
    hash.add(var_idx);
  hash.add(p_manager.non_fixed_var_idxs().size());
  for (std::size_t var_idx : p_manager.non_fixed_var_idxs())
    hash.add(var_idx);
  hash.add(p_manager.con_is_equality().size());
  for (bool is_equality : p_manager.con_is_equality())
    hash.add(is_equality);
  hash.add(p_manager.var_obj_cost().size());
  for (double cost : p_manager.var_obj_cost())
    hash.add_double(cost);
  for (std::size_t var_idx = 0; var_idx < p_manager.var_num(); ++var_idx)
    hash.add(p_manager.var_id_to_obj_idx(var_idx));
  return hash.value();
}

bool test_file_preparation_and_guards()
{
  bool null_rejected = false;
  try
  {
    Local_MIP invalid_solver{
        std::shared_ptr<const Prepared_Model>()};
  }
  catch (const std::invalid_argument&)
  {
    null_rejected = true;
  }

  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = Prepared_Model::from_file(TEST_MPS_PATH, options);

  bool ok = true;
  ok &= check(null_rejected, "Null prepared model should be rejected");
  ok &= check(prepared != nullptr, "File preparation should return a model");
  ok &= check(prepared->model_manager().var_num() > 0,
              "Prepared file model should contain variables");

  auto prepared_lp = Prepared_Model::from_file(TEST_LP_PATH, options);
  ok &= check(prepared_lp->model_manager().var_num() > 0,
              "LP file preparation should contain variables");

  Local_MIP solver(prepared);
  ok &= check(solver.get_model_manager() == &prepared->model_manager(),
              "Solver should read the shared Model_Manager without a copy");

  bool model_change_rejected = false;
  try
  {
    solver.set_split_eq(false);
  }
  catch (const std::logic_error& error)
  {
    model_change_rejected =
        std::string(error.what()).find("cannot modify model") !=
        std::string::npos;
  }
  ok &= check(model_change_rejected,
              "Prepared model configuration should be immutable");

  try
  {
    solver.set_opt_tolerance(1e-5);
  }
  catch (...)
  {
    ok &= check(false,
                "Search-only optimality tolerance should remain configurable");
  }
  return ok;
}

bool test_builder_preparation()
{
  Model_Builder builder;
  builder.set_sense(Model_Builder::Sense::maximize);
  const int x =
      builder.add_var("x", 0.0, 1.0, 1.0, Var_Type::binary);
  if (x < 0)
    return check(false, "Builder should add a variable");
  if (builder.add_con(k_neg_inf,
                      1.0,
                      std::vector<int>{x},
                      std::vector<double>{1.0}) < 0)
  {
    return check(false, "Builder should add a constraint");
  }

  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = builder.prepare(options);
  Local_MIP solver(prepared);
  solver.set_log_obj(false);
  solver.set_time_limit(0.1);
  solver.run();

  bool ok = true;
  ok &= check(solver.is_feasible(),
              "Solver should solve an in-memory prepared model");
  ok &= check(!solver.get_solution().empty() &&
                  solver.get_solution()[static_cast<std::size_t>(x)] == 1.0,
              "Prepared maximize model should preserve objective direction");

  Model_Builder tolerance_builder;
  const int y = tolerance_builder.add_var(
      "y", 0.0, 2.0, 5e-6, Var_Type::real);
  tolerance_builder.add_con(1.0,
                            1.0 + 5e-6,
                            std::vector<int>{y},
                            std::vector<double>{1.0});
  Model_Prepare_Options tolerance_options;
  tolerance_options.feas_tolerance = 1e-5;
  tolerance_options.zero_tolerance = 1e-5;
  tolerance_options.bound_strengthen = 0;
  tolerance_options.split_eq = false;
  auto tolerance_model = tolerance_builder.prepare(tolerance_options);
  ok &= check(tolerance_model->model_manager().obj().term_num() == 0,
              "Builder preparation should apply option zero_tolerance");
  ok &= check(tolerance_model->model_manager().con(1).is_equality(),
              "Builder preparation should apply option feas_tolerance");
  const auto& tolerance_manager = tolerance_model->model_manager();
  ok &= check(tolerance_manager.var_in_bound(
                  tolerance_manager.var(static_cast<std::size_t>(y)),
                  2.0 + 5e-6),
              "Prepared variables should retain the model feasibility "
              "tolerance outside run()");
  ok &= check(tolerance_manager.feas_tolerance() ==
                      tolerance_options.feas_tolerance &&
                  tolerance_manager.zero_tolerance() ==
                      tolerance_options.zero_tolerance,
              "Prepared model should own its numerical tolerances");

  Model_Builder bounds_builder;
  const int z = bounds_builder.add_var(
      "z", 0.0, 2.0, 0.0, Var_Type::real);
  ok &= check(bounds_builder.add_con(
                  1.0 + 0.5 * k_default_feas_tolerance,
                  1.0,
                  std::vector<int>{z},
                  std::vector<double>{1.0}) < 0,
              "Model API should reject inverted bounds independently of "
              "prepare-time tolerance");

  Model_Builder exact_builder;
  const int fixed = exact_builder.add_var(
      "fixed", 2.0, 2.0, 0.0, Var_Type::real);
  exact_builder.add_con(2.0,
                        2.0,
                        std::vector<int>{fixed},
                        std::vector<double>{1.0});
  Model_Prepare_Options exact_options;
  exact_options.feas_tolerance = 0.0;
  exact_options.bound_strengthen = 0;
  exact_options.split_eq = false;
  auto exact_model = exact_builder.prepare(exact_options);
  const auto& exact_manager = exact_model->model_manager();
  ok &= check(exact_manager.var(static_cast<std::size_t>(fixed)).type() ==
                  Var_Type::fixed,
              "Exact fixed bounds should remain fixed at zero tolerance");
  ok &= check(exact_model->model_manager().con(1).is_equality(),
              "Exact equal constraint bounds should remain an equality at "
              "zero tolerance");

  Model_Builder zero_builder;
  const int free_var = zero_builder.add_var(
      "free", 0.0, 1.0, 0.0, Var_Type::real);
  zero_builder.add_con(0.0,
                       0.0,
                       std::vector<int>{free_var},
                       std::vector<double>{0.0});
  Model_Prepare_Options zero_options;
  zero_options.feas_tolerance = 0.0;
  zero_options.zero_tolerance = 0.0;
  zero_options.bound_strengthen = 0;
  zero_options.split_eq = false;
  auto zero_model = zero_builder.prepare(zero_options);
  ok &= check(zero_model->model_manager().con(1).term_num() == 0,
              "Exact zero coefficients should be removed at zero tolerance");
  return ok;
}

bool test_prepared_parameter_files()
{
  Model_Prepare_Options options;
  options.feas_tolerance = 1e-5;
  options.zero_tolerance = 1e-7;
  options.bound_strengthen = 0;
  options.split_eq = false;
  auto prepared = Prepared_Model::from_file(TEST_MPS_PATH, options);
  Local_MIP solver(prepared);

  const char* matching_file = "tmp_prepared_matching.set";
  std::FILE* fp = std::fopen(matching_file, "w");
  if (fp == nullptr)
    return check(false, "Should create matching prepared parameter file");
  std::fprintf(fp, "time_limit = 0.2\n");
  std::fprintf(fp, "random_seed = 17\n");
  std::fprintf(fp, "feas_tolerance = 1e-5\n");
  std::fprintf(fp, "zero_tolerance = 1e-7\n");
  std::fprintf(fp, "bound_strengthen = 0\n");
  std::fprintf(fp, "split_eq = 0\n");
  std::fclose(fp);

  bool ok = true;
  try
  {
    solver.set_param_set_file(matching_file);
  }
  catch (...)
  {
    ok &= check(false,
                "Matching prepared-model parameters should be accepted");
  }
  ok &= check(solver.m_param_set_file == matching_file,
              "Accepted prepared parameter file should be recorded");
  ok &= check(solver.m_time_limit == 0.2,
              "Accepted prepared parameter file should apply search settings");

  const double time_before = solver.m_time_limit;
  const auto rng_before = solver.m_local_search->m_rng;
  const std::string path_before = solver.m_param_set_file;
  const char* conflicting_file = "tmp_prepared_conflicting.set";
  fp = std::fopen(conflicting_file, "w");
  if (fp == nullptr)
  {
    std::remove(matching_file);
    return check(false,
                 "Should create conflicting prepared parameter file");
  }
  std::fprintf(fp, "time_limit = 7\n");
  std::fprintf(fp, "random_seed = 99\n");
  std::fprintf(fp, "feas_tolerance = 2e-5\n");
  std::fclose(fp);

  bool conflict_rejected = false;
  try
  {
    solver.set_param_set_file(conflicting_file);
  }
  catch (const std::logic_error&)
  {
    conflict_rejected = true;
  }
  ok &= check(conflict_rejected,
              "Conflicting prepared-model parameters should be rejected");
  ok &= check(solver.m_time_limit == time_before &&
                  solver.m_local_search->m_rng == rng_before &&
                  solver.m_param_set_file == path_before,
              "Rejected prepared parameter file should not partially modify "
              "search settings");

  std::remove(matching_file);
  std::remove(conflicting_file);
  return ok;
}

bool test_parallel_shared_search()
{
  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = Prepared_Model::from_file(TEST_MPS_PATH, options);
  std::weak_ptr<const Prepared_Model> prepared_weak = prepared;
  const Model_Manager& manager = prepared->model_manager();
  const void* model_address = &manager;
  const std::uint64_t hash_before = structure_hash(manager);

  constexpr std::size_t k_solver_num = 4;
  std::vector<std::unique_ptr<Local_MIP>> solvers;
  solvers.reserve(k_solver_num);
  for (std::size_t idx = 0; idx < k_solver_num; ++idx)
  {
    auto solver = std::make_unique<Local_MIP>(prepared);
    solver->set_random_seed(static_cast<std::uint32_t>(idx + 1));
    solver->set_time_limit(0.1);
    solver->set_log_obj(false);
    solver->reset_default_neighbor_list();
    solvers.push_back(std::move(solver));
  }

  bool ok = true;
  for (std::size_t idx = 0; idx < k_solver_num; ++idx)
  {
    ok &= check(solvers[idx]->get_model_manager() == model_address,
                "All solvers should share one model address");
    for (std::size_t other = 0; other < idx; ++other)
    {
      ok &= check(solvers[idx]->m_local_search.get() !=
                      solvers[other]->m_local_search.get(),
                  "Search state must be solver-local");
      ok &= check(&solvers[idx]->m_local_search->m_rng !=
                      &solvers[other]->m_local_search->m_rng,
                  "RNG state must be solver-local");
      ok &= check(&solvers[idx]
                           ->m_local_search
                           ->m_explore_neighbor_list[0]
                           .m_bms_idxs !=
                      &solvers[other]
                           ->m_local_search
                           ->m_explore_neighbor_list[0]
                           .m_bms_idxs,
                  "Sampling buffers must be solver-local");
    }
  }

  prepared.reset();
  std::vector<std::exception_ptr> errors(k_solver_num);
  std::vector<std::thread> workers;
  workers.reserve(k_solver_num);
  for (std::size_t idx = 0; idx < k_solver_num; ++idx)
  {
    workers.emplace_back(
        [&, idx]()
        {
          try
          {
            solvers[idx]->run();
          }
          catch (...)
          {
            errors[idx] = std::current_exception();
          }
        });
  }
  for (auto& worker : workers)
    worker.join();

  for (const auto& error : errors)
    ok &= check(error == nullptr,
                "Every shared-model solver should finish without throwing");
  ok &= check(!prepared_weak.expired(),
              "Solver should keep the prepared model alive after owner release");
  ok &= check(structure_hash(*solvers[0]->get_model_manager()) == hash_before,
              "Concurrent search must preserve prepared model structure");
  return ok;
}

bool test_solver_local_tolerance_context()
{
  Model_Builder builder;
  const int x =
      builder.add_var("x", 0.0, 1.0, 1.0, Var_Type::binary);
  builder.add_con(k_neg_inf,
                  1.0,
                  std::vector<int>{x},
                  std::vector<double>{1.0});
  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = builder.prepare(options);

  constexpr double k_direct_opt = 3e-3;
  double observed_direct_opt = -1.0;
  Local_Search direct_search(&prepared->model_manager(), k_direct_opt);
  observed_direct_opt = direct_search.m_readonly_ctx.m_opt_tolerance;

  Local_MIP first(prepared);
  Local_MIP second(prepared);
  constexpr double k_first_opt = 1e-3;
  constexpr double k_second_opt = 2e-3;
  first.set_opt_tolerance(k_first_opt);
  second.set_opt_tolerance(k_second_opt);
  first.set_time_limit(0.05);
  second.set_time_limit(0.05);
  first.set_log_obj(false);
  second.set_log_obj(false);

  double observed_first = -1.0;
  double observed_second = -1.0;
  first.set_start_cbk(
      [&](Start::Start_Ctx& ctx, void*)
      { observed_first = ctx.m_shared.m_opt_tolerance; });
  second.set_start_cbk(
      [&](Start::Start_Ctx& ctx, void*)
      { observed_second = ctx.m_shared.m_opt_tolerance; });

  std::exception_ptr first_error;
  std::exception_ptr second_error;
  std::thread first_worker(
      [&]()
      {
        try
        {
          first.run();
        }
        catch (...)
        {
          first_error = std::current_exception();
        }
      });
  std::thread second_worker(
      [&]()
      {
        try
        {
          second.run();
        }
        catch (...)
        {
          second_error = std::current_exception();
        }
      });
  first_worker.join();
  second_worker.join();

  bool ok = true;
  ok &= check(observed_direct_opt == k_direct_opt,
              "Direct Local_Search should own its explicit optimality "
              "tolerance");
  ok &= check(first_error == nullptr && second_error == nullptr,
              "Tolerance-isolation runs should complete without throwing");
  ok &= check(observed_first == k_first_opt,
              "First callback should expose its solver tolerance");
  ok &= check(observed_second == k_second_opt,
              "Second callback should expose its solver tolerance");
  return ok;
}

bool test_prepared_solver_is_one_shot()
{
  Model_Builder builder;
  const int x =
      builder.add_var("x", 0.0, 1.0, 1.0, Var_Type::binary);
  builder.add_con(k_neg_inf,
                  1.0,
                  std::vector<int>{x},
                  std::vector<double>{1.0});
  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  auto prepared = builder.prepare(options);

  Local_MIP solver(prepared);
  solver.set_time_limit(0.05);
  solver.set_log_obj(false);
  solver.run();

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

  Local_MIP replacement(prepared);
  replacement.set_time_limit(0.05);
  replacement.set_log_obj(false);
  replacement.run();

  bool ok = true;
  ok &= check(second_run_rejected,
              "A prepared-model solver should permit only one run");
  ok &= check(replacement.is_feasible(),
              "A new solver should reuse the same prepared model");
  return ok;
}

bool test_distinct_model_tolerance_contexts()
{
  Model_Builder builder;
  const int x =
      builder.add_var("x", 0.0, 1.0, 1.0, Var_Type::binary);
  const int y = builder.add_var(
      "y", 0.0, 0.9995, 0.0, Var_Type::general_integer);
  builder.add_con(k_neg_inf,
                  1.0,
                  std::vector<int>{x},
                  std::vector<double>{1.0});

  Model_Prepare_Options first_options;
  first_options.feas_tolerance = 1e-3;
  first_options.zero_tolerance = 1e-5;
  first_options.bound_strengthen = 0;
  Model_Prepare_Options second_options;
  second_options.feas_tolerance = 1e-5;
  second_options.zero_tolerance = 1e-7;
  second_options.bound_strengthen = 0;
  auto first_model = builder.prepare(first_options);
  auto second_model = builder.prepare(second_options);

  bool ok = true;
  const auto& first_manager = first_model->model_manager();
  const auto& second_manager = second_model->model_manager();
  ok &= check(first_manager.var_in_bound(
                  first_manager.var(static_cast<std::size_t>(x)), 1.0005),
              "Large-tolerance prepared variable should accept nearby value");
  ok &= check(!second_manager.var_in_bound(
                   second_manager.var(static_cast<std::size_t>(x)), 1.0005),
              "Small-tolerance prepared variable should reject nearby value");
  ok &= check(first_manager.var(static_cast<std::size_t>(y)).upper_bound() ==
                  1.0,
              "Large model tolerance should round a nearby integer bound up");
  ok &= check(second_manager.var(static_cast<std::size_t>(y)).upper_bound() ==
                  0.0,
              "Small model tolerance should round the same integer bound down");
  double direct_feas = -1.0;
  double direct_zero = -1.0;
  Local_Search direct_search(&first_manager);
  direct_search.set_start_cbk(
      [&](Start::Start_Ctx& ctx, void*)
      {
        direct_feas = ctx.m_shared.m_model_manager.feas_tolerance();
        direct_zero = ctx.m_shared.m_model_manager.zero_tolerance();
      });
  direct_search.terminate();
  direct_search.run_search();
  ok &= check(direct_feas == first_options.feas_tolerance &&
                  direct_zero == first_options.zero_tolerance,
              "Direct Local_Search callback should expose model tolerances");
  Local_MIP first(first_model);
  Local_MIP second(second_model);
  first.set_time_limit(0.05);
  second.set_time_limit(0.05);
  first.set_log_obj(false);
  second.set_log_obj(false);
  double first_feas = -1.0;
  double first_zero = -1.0;
  double second_feas = -1.0;
  double second_zero = -1.0;
  first.set_start_cbk(
      [&](Start::Start_Ctx& ctx, void*)
      {
        first_feas = ctx.m_shared.m_model_manager.feas_tolerance();
        first_zero = ctx.m_shared.m_model_manager.zero_tolerance();
      });
  second.set_start_cbk(
      [&](Start::Start_Ctx& ctx, void*)
      {
        second_feas = ctx.m_shared.m_model_manager.feas_tolerance();
        second_zero = ctx.m_shared.m_model_manager.zero_tolerance();
      });

  std::exception_ptr first_error;
  std::exception_ptr second_error;
  std::thread first_worker(
      [&]()
      {
        try
        {
          first.run();
        }
        catch (...)
        {
          first_error = std::current_exception();
        }
      });
  std::thread second_worker(
      [&]()
      {
        try
        {
          second.run();
        }
        catch (...)
        {
          second_error = std::current_exception();
        }
      });
  first_worker.join();
  second_worker.join();

  ok &= check(first_error == nullptr && second_error == nullptr,
              "Distinct-tolerance concurrent runs should complete");
  ok &= check(first_feas == first_options.feas_tolerance &&
                  first_zero == first_options.zero_tolerance,
              "First search context should expose its model tolerances");
  ok &= check(second_feas == second_options.feas_tolerance &&
                  second_zero == second_options.zero_tolerance,
              "Second search context should expose its model tolerances");
  return ok;
}

bool test_invalid_options()
{
  static_assert(std::is_same_v<Model_Builder, Model_API>);

  auto rejects = [](const Model_Prepare_Options& options)
  {
    try
    {
      Prepared_Model::from_file(TEST_MPS_PATH, options);
    }
    catch (const std::invalid_argument&)
    {
      return true;
    }
    return false;
  };

  bool ok = true;
  Model_Prepare_Options options;
  options.bound_strengthen = 3;
  ok &= check(rejects(options),
              "Invalid bound-strengthening level should be rejected");

  options = Model_Prepare_Options();
  options.feas_tolerance =
      std::numeric_limits<double>::quiet_NaN();
  ok &= check(rejects(options),
              "Non-finite feasibility tolerance should be rejected");

  options = Model_Prepare_Options();
  options.zero_tolerance = k_max_zero_tolerance * 2.0;
  ok &= check(rejects(options),
              "Out-of-range zero tolerance should be rejected");

  Model_Builder invalid_builder;
  const int x = invalid_builder.add_var(
      "x", 0.0, 1.0, 0.0, Var_Type::real);
  invalid_builder.add_con(
      k_neg_inf,
      1.0,
      std::vector<int>{x},
      std::vector<double>{std::numeric_limits<double>::quiet_NaN()});
  bool invalid_data_rejected = false;
  try
  {
    invalid_builder.prepare();
  }
  catch (const std::invalid_argument&)
  {
    invalid_data_rejected = true;
  }
  ok &= check(invalid_data_rejected,
              "Prepared model should reject non-finite coefficients");

  bool empty_model_rejected = false;
  try
  {
    Model_Builder().prepare();
  }
  catch (const std::invalid_argument&)
  {
    empty_model_rejected = true;
  }
  ok &= check(empty_model_rejected,
              "Prepared model should reject an empty model");
  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_file_preparation_and_guards();
  ok &= test_builder_preparation();
  ok &= test_prepared_parameter_files();
  ok &= test_parallel_shared_search();
  ok &= test_solver_local_tolerance_context();
  ok &= test_prepared_solver_is_one_shot();
  ok &= test_distinct_model_tolerance_contexts();
  ok &= test_invalid_options();
  if (!ok)
    return EXIT_FAILURE;
  std::printf("c shared Prepared_Model tests PASSED.\n");
  return EXIT_SUCCESS;
}
