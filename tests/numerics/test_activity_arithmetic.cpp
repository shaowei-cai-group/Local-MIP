/*=====================================================================================

    Filename:     test_activity_arithmetic.cpp

    Description:  Exact-double activity arithmetic and certification tests
        Version:  2.0

=====================================================================================*/

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define private public
#define protected public
#include "Local_MIP.h"
#include "local_search/Local_Search.h"
#undef private
#undef protected

#include "model_api/Model_Builder.h"

namespace
{

constexpr double k_exact_limit = 9007199254740992.0; // 2^53

bool check(bool p_condition, const char* p_message)
{
  if (!p_condition)
  {
    std::fprintf(stderr, "ERROR: %s\n", p_message);
    return false;
  }
  return true;
}

std::shared_ptr<const Prepared_Model>
prepare_model(const Model_Builder& p_builder)
{
  Model_Prepare_Options options;
  options.bound_strengthen = 0;
  options.split_eq = false;
  return p_builder.prepare(options);
}

Local_Search* initialize_search(Local_MIP& p_solver)
{
  Local_Search* search = p_solver.m_local_search.get();
  search->init_data();
  return search;
}

bool test_safe_binary_model_and_period_semantics()
{
  Model_Builder binary_builder;
  const int binary_x = binary_builder.add_var(
      "binary_x", 0.0, 1.0, 2.0, Var_Type::binary);
  binary_builder.add_con(k_neg_inf,
                         5.0,
                         std::vector<int>{binary_x},
                         std::vector<double>{5.0});
  Local_MIP binary_solver(prepare_model(binary_builder));
  Local_Search* binary_search = initialize_search(binary_solver);
  bool ok = true;
  ok &= check(binary_search->m_use_exact_double_activity,
              "small binary model should use exact double");
  binary_search->init_state();
  binary_search->apply_move(static_cast<size_t>(binary_x), 1.0);
  ok &= check(binary_search->m_con_activity[0] == 2.0 &&
                  binary_search->m_con_activity[1] == 5.0,
              "small binary activity updates should be exact");

  Model_Builder builder;
  const int x = builder.add_var(
      "x", 0.0, 100.0, 2.0, Var_Type::general_integer);
  builder.add_con(k_neg_inf,
                  500.0,
                  std::vector<int>{x},
                  std::vector<double>{5.0});

  Local_MIP solver(prepare_model(builder));
  Local_Search* search = initialize_search(solver);
  ok &= check(search->m_use_exact_double_activity,
              "bounded integer model should use exact double");
  search->init_state();
  for (int value = 1; value <= 50; ++value)
  {
    search->apply_move(static_cast<size_t>(x), 1.0);
    ok &= check(search->m_var_current_value[static_cast<size_t>(x)] == value,
                "integer move should remain exact");
    ok &= check(search->m_con_activity[0] == 2.0 * value,
                "objective activity should remain exact");
    ok &= check(search->m_con_activity[1] == 5.0 * value,
                "constraint activity should remain exact");
  }
  ok &= check(search->m_activity_dirty,
              "T=infinity must retain the activity dirty state");
  ok &= check(search->m_activity_hits == 0,
              "T=infinity must not grow the period counter");
  const double activity_before_refresh = search->m_con_activity[1];
  search->refresh_activities();
  ok &= check(!search->m_activity_dirty,
              "full recomputation should clear the dirty state");
  ok &= check(search->m_con_activity[1] == activity_before_refresh,
              "exact refresh should be bitwise stable");

  Local_MIP configured_solver(prepare_model(builder));
  configured_solver.set_activity_period(2);
  Local_Search* configured_search = initialize_search(configured_solver);
  ok &= check(configured_search->m_use_exact_double_activity,
              "configured period must not disable exact arithmetic");
  configured_search->init_state();
  configured_search->apply_move(static_cast<size_t>(x), 1.0);
  configured_search->apply_move(static_cast<size_t>(x), 1.0);
  ok &= check(configured_search->m_activity_hits == 0 &&
                  configured_search->m_activity_dirty,
              "certified model must keep T=infinity after configuration");
  return ok;
}

bool test_certificate_rejections()
{
  bool ok = true;

  {
    Model_Builder builder;
    const int x = builder.add_var(
        "x", 0.0, k_inf, 0.0, Var_Type::general_integer);
    builder.add_con(k_neg_inf,
                    0.0,
                    std::vector<int>{x},
                    std::vector<double>{1.0});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "unbounded relevant variable must be rejected");
  }

  {
    Model_Builder builder;
    const int x =
        builder.add_var("x", 0.5, 0.5, 0.0, Var_Type::real);
    builder.add_con(k_neg_inf,
                    1.0,
                    std::vector<int>{x},
                    std::vector<double>{1.0});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "non-integer fixed bound must be rejected");
  }

  {
    Model_Builder builder;
    const int x =
        builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
    const int y =
        builder.add_var("y", 0.0, 1.0, 0.0, Var_Type::binary);
    builder.add_con(k_neg_inf,
                    k_exact_limit,
                    std::vector<int>{x, y},
                    std::vector<double>{k_exact_limit, 1.0});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "2^53*x+y must fail the absolute-sum certificate");
  }

  {
    Model_Builder builder;
    const int x = builder.add_var(
        "x", 0.0, 2.0, 0.0, Var_Type::general_integer);
    builder.add_con(k_neg_inf,
                    k_exact_limit,
                    std::vector<int>{x},
                    std::vector<double>{k_exact_limit});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "single activity product above 2^53 must be rejected");
  }

  {
    Model_Builder builder;
    const int x =
        builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
    const int y =
        builder.add_var("y", 0.0, 1.0, 0.0, Var_Type::binary);
    const int z =
        builder.add_var("z", 0.0, 1.0, 0.0, Var_Type::binary);
    builder.add_con(k_neg_inf,
                    k_exact_limit,
                    std::vector<int>{x, y, z},
                    std::vector<double>{
                        k_exact_limit, 1.0, -k_exact_limit});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "large cancellation must not bypass the absolute-sum bound");
  }

  {
    Model_Builder builder;
    const int x = builder.add_var("x",
                                  -k_exact_limit,
                                  k_exact_limit,
                                  0.0,
                                  Var_Type::general_integer);
    builder.add_con(k_neg_inf,
                    k_exact_limit,
                    std::vector<int>{x},
                    std::vector<double>{1.0});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "domain width above 2^53 must be rejected");
  }

  {
    Model_Builder builder;
    const int x = builder.add_var(
        "x", -1.0, 1.0, 0.0, Var_Type::general_integer);
    builder.add_con(k_neg_inf,
                    k_exact_limit,
                    std::vector<int>{x},
                    std::vector<double>{k_exact_limit});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(
        !search->m_use_exact_double_activity,
        "incremental coefficient-times-domain-width above 2^53 must be "
        "rejected");
  }

  {
    Model_Builder builder;
    const int x =
        builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::real);
    builder.add_con(k_neg_inf,
                    1.0,
                    std::vector<int>{x},
                    std::vector<double>{1.0});
    Local_MIP solver(prepare_model(builder));
    solver.set_activity_period(2);
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "non-fixed real variable must be rejected");
    search->init_state();
    search->apply_move(static_cast<size_t>(x), 0.25);
    ok &= check(search->m_activity_hits == 1 &&
                    search->m_activity_dirty,
                "fallback model must advance its configured period");
    search->apply_move(static_cast<size_t>(x), 0.25);
    ok &= check(search->m_activity_hits == 0 &&
                    !search->m_activity_dirty,
                "fallback model must recompute at its configured period");
  }

  {
    Model_Builder builder;
    const int x =
        builder.add_var("x", 0.0, 1.0, 0.0, Var_Type::binary);
    builder.add_con(k_neg_inf,
                    1.0,
                    std::vector<int>{x},
                    std::vector<double>{0.5});
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "fractional coefficient must be rejected");
  }

  {
    Model_Builder builder;
    builder.add_var(
        "x", 0.0, 1.0, k_exact_limit, Var_Type::binary);
    builder.add_var("y", 0.0, 1.0, 1.0, Var_Type::binary);
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "objective absolute-sum above 2^53 must be rejected");
  }

  return ok;
}

bool test_long_exact_move_sequence()
{
  Model_Builder builder;
  const int x = builder.add_var(
      "x", -1000.0, 1000.0, 3.0, Var_Type::general_integer);
  const int y = builder.add_var(
      "y", -1000.0, 1000.0, -7.0, Var_Type::general_integer);
  builder.add_con(k_neg_inf,
                  100000.0,
                  std::vector<int>{x, y},
                  std::vector<double>{11.0, 13.0});

  Local_MIP solver(prepare_model(builder));
  Local_Search* search = initialize_search(solver);
  bool ok = true;
  ok &= check(search->m_use_exact_double_activity,
              "bounded integer sequence model should be certified");
  search->init_state();

  int64_t x_reference = 0;
  int64_t y_reference = 0;
  constexpr int k_steps = 20000;
  constexpr int k_phase_length = 100;
  for (int step = 0; step < k_steps; ++step)
  {
    const int phase = (step / k_phase_length) % 4;
    if (phase == 0)
    {
      search->apply_move(static_cast<size_t>(x), 1.0);
      ++x_reference;
    }
    else if (phase == 1)
    {
      search->apply_move(static_cast<size_t>(y), 1.0);
      ++y_reference;
    }
    else if (phase == 2)
    {
      search->apply_move(static_cast<size_t>(x), -1.0);
      --x_reference;
    }
    else
    {
      search->apply_move(static_cast<size_t>(y), -1.0);
      --y_reference;
    }

    const int64_t objective_reference =
        3 * x_reference - 7 * y_reference;
    const int64_t constraint_reference =
        11 * x_reference + 13 * y_reference;
    if (!check(search->m_var_current_value[static_cast<size_t>(x)] ==
                   static_cast<double>(x_reference) &&
                   search->m_var_current_value[static_cast<size_t>(y)] ==
                       static_cast<double>(y_reference) &&
                   search->m_con_activity[0] ==
                       static_cast<double>(objective_reference) &&
                   search->m_con_activity[1] ==
                       static_cast<double>(constraint_reference),
               "long exact move sequence diverged from integer reference"))
    {
      std::fprintf(stderr, "Failure occurred at move %d\n", step + 1);
      return false;
    }
  }

  ok &= check(x_reference == 0 && y_reference == 0,
              "long move sequence should return to its start");
  ok &= check(search->m_activity_hits == 0 && search->m_activity_dirty,
              "long exact move sequence must retain T=infinity semantics");
  return ok;
}

bool test_extended_precision_certification()
{
  if (std::numeric_limits<long double>::digits <=
      std::numeric_limits<double>::digits)
    return true;

  Model_Builder builder;
  const int x =
      builder.add_var("x", 0.0, 1.0, k_exact_limit, Var_Type::binary);
  const int y =
      builder.add_var("y", 0.0, 1.0, 1.0, Var_Type::binary);
  builder.add_con(k_neg_inf,
                  k_exact_limit,
                  std::vector<int>{x, y},
                  std::vector<double>{k_exact_limit, 1.0});
  Local_MIP solver(prepare_model(builder));
  Local_Search* search = initialize_search(solver);
  search->init_state();
  search->m_var_current_value[static_cast<size_t>(x)] = 1.0;
  search->m_var_current_value[static_cast<size_t>(y)] = 1.0;
  search->m_con_constant[0] = k_exact_limit;
  search->refresh_activities();

  bool ok = true;
  ok &= check(search->m_con_activity[0] == k_exact_limit,
              "stored objective should expose the boundary rounding case");
  ok &= check(!search->m_current_obj_breakthrough,
              "objective comparison must use the wide recomputed value");
  ok &= check(search->m_con_activity[1] == k_exact_limit,
              "stored double should expose the boundary rounding case");
  ok &= check(search->m_con_pos_in_unsat_idxs[1] != SIZE_MAX,
              "constraint classification must use the wide recomputed value");
  search->m_var_best_value = search->m_var_current_value;
  ok &= check(!search->verify_solution(),
              "final constraint verification must compare in long double");

  search->m_con_constant[1] = k_inf;
  search->m_best_obj = k_exact_limit;
  ok &= check(search->verify_solution(),
              "objective verification must match double objective storage");
  search->m_con_constant[1] = k_exact_limit;

  search->apply_move(static_cast<size_t>(y), -1.0);
  ok &= check(search->m_con_pos_in_sat_idxs[1] != SIZE_MAX,
              "move update should preserve stored-state consistency");
  search->apply_move(static_cast<size_t>(y), 1.0);
  if (search->m_con_unsat_idxs.empty())
  {
    ok &= check(search->m_activity_dirty,
                "a rounded fallback state must require certification");
    search->refresh_activities();
  }
  ok &= check(search->m_con_pos_in_unsat_idxs[1] != SIZE_MAX,
              "wide certification must reject a false-feasible round trip");

  Model_Builder equality_builder;
  const int eq_x = equality_builder.add_var(
      "eq_x", 0.0, 1.0, 0.0, Var_Type::binary);
  const int eq_y = equality_builder.add_var(
      "eq_y", 0.0, 1.0, 0.0, Var_Type::binary);
  equality_builder.add_con(k_exact_limit,
                           k_exact_limit,
                           std::vector<int>{eq_x, eq_y},
                           std::vector<double>{k_exact_limit, 1.0});
  Local_MIP equality_solver(prepare_model(equality_builder));
  Local_Search* equality_search = initialize_search(equality_solver);
  equality_search->init_state();
  equality_search->m_var_current_value[static_cast<size_t>(eq_x)] = 1.0;
  equality_search->m_var_current_value[static_cast<size_t>(eq_y)] = 1.0;
  equality_search->refresh_activities();
  ok &= check(equality_search->m_con_pos_in_unsat_idxs[1] != SIZE_MAX,
              "wide equality refresh must reject a rounded boundary value");
  equality_search->apply_move(static_cast<size_t>(eq_y), -1.0);
  equality_search->refresh_activities();
  ok &= check(equality_search->m_con_pos_in_sat_idxs[1] != SIZE_MAX,
              "wide equality refresh must accept the exact boundary value");

  Model_Builder incremental_builder;
  const int inc_x = incremental_builder.add_var(
      "inc_x", 0.0, 1.0, 0.0, Var_Type::binary);
  const int inc_y = incremental_builder.add_var(
      "inc_y", 0.0, 1.0, 0.0, Var_Type::binary);
  incremental_builder.add_con(
      k_neg_inf,
      k_exact_limit,
      std::vector<int>{inc_x, inc_y},
      std::vector<double>{k_exact_limit, 1.0});
  Local_MIP incremental_solver(prepare_model(incremental_builder));
  Local_Search* incremental_search = initialize_search(incremental_solver);
  incremental_search->init_state();
  incremental_search->apply_move(static_cast<size_t>(inc_x), 1.0);
  ok &= check(incremental_search->m_con_pos_in_sat_idxs[1] != SIZE_MAX,
              "exact boundary activity should be feasible");
  incremental_search->apply_move(static_cast<size_t>(inc_y), 1.0);
  ok &= check(incremental_search->m_con_activity[1] == k_exact_limit,
              "incremental boundary case should expose double rounding");
  ok &= check(incremental_search->m_con_pos_in_unsat_idxs[1] != SIZE_MAX,
              "incremental classification must precede double write-back");
  return ok;
}

bool test_objective_only_dispatch()
{
  bool ok = true;
  {
    Model_Builder builder;
    builder.add_var("x", 0.0, 1.0, -5.0, Var_Type::binary);
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(search->m_use_exact_double_activity,
                "integer objective-only model should be certified");
    ok &= check(search->solve_objective_only(),
                "objective-only fast path should run");
    ok &= check(search->m_best_obj == -5.0,
                "exact objective-only accumulation should be correct");
  }
  {
    Model_Builder builder;
    builder.add_var("x", 0.0, 1.0, -0.5, Var_Type::real);
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "fractional objective-only model should fall back");
    ok &= check(search->solve_objective_only(),
                "fallback objective-only fast path should run");
    ok &= check(search->m_best_obj == -0.5,
                "long-double objective-only accumulation should be correct");
  }
  {
    Model_Builder builder;
    builder.add_var(
        "x", 0.0, 1.0, -k_exact_limit, Var_Type::binary);
    builder.add_var("y", 0.0, 1.0, -1.0, Var_Type::binary);
    Local_MIP solver(prepare_model(builder));
    Local_Search* search = initialize_search(solver);
    ok &= check(!search->m_use_exact_double_activity,
                "wide objective-only model should fall back");
    ok &= check(search->solve_objective_only(),
                "wide objective-only fast path should run");
    ok &= check(search->finalize_result() && search->m_is_found_feasible,
                "rounded double objective storage must still verify");
  }
  return ok;
}

} // namespace

int main()
{
  bool ok = true;
  ok &= test_safe_binary_model_and_period_semantics();
  ok &= test_certificate_rejections();
  ok &= test_long_exact_move_sequence();
  ok &= test_extended_precision_certification();
  ok &= test_objective_only_dispatch();
  if (!ok)
    return 1;
  std::printf("All activity arithmetic tests passed.\n");
  return 0;
}
