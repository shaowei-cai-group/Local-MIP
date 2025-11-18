/*=====================================================================================

    Filename:     scoring.h

    Description:  Scoring functions for move evaluation
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include "../context/context.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

class Model_Manager;

class Scoring
{
public:
  class Lift_Ctx
  {
  public:
    Lift_Ctx(const Readonly_Ctx& p_shared,
             std::mt19937& p_rng,
             double& p_best_lift_score,
             size_t& p_current_best_var_idx,
             double& p_current_best_delta,
             size_t& p_current_best_age);

    const Readonly_Ctx& m_shared;

    std::mt19937& m_rng;

    double& m_best_lift_score;

    size_t& m_best_var_idx;

    double& m_best_delta;

    size_t& m_best_age;
  };

  class Neighbor_Ctx
  {
  public:
    Neighbor_Ctx(const Readonly_Ctx& p_shared,
                 std::vector<uint32_t>& p_binary_op_stamp,
                 uint32_t& p_binary_op_stamp_token,
                 long& p_current_neighbor_score,
                 long& p_current_neighbor_subscore,
                 size_t& p_current_best_age,
                 size_t& p_current_best_var_idx,
                 double& p_current_best_delta);

    const Readonly_Ctx& m_shared;

    std::vector<uint32_t>& m_binary_op_stamp;

    uint32_t& m_binary_op_stamp_token;

    long& m_best_neighbor_score;

    long& m_best_neighbor_subscore;

    size_t& m_best_age;

    size_t& m_best_var_idx;

    double& m_best_delta;
  };

  using Lift_Cbk = std::function<void(Lift_Ctx&, size_t, double, void*)>;

  using Neighbor_Cbk =
      std::function<void(Neighbor_Ctx&, size_t, double, void*)>;

  Scoring()
      : m_lift_cbk(nullptr), m_lift_user_data(nullptr),
        m_neighbor_cbk(nullptr), m_neighbor_user_data(nullptr),
        m_lift_method(Lift_Method::lift_age),
        m_neighbor_method(Neighbor_Method::progress_bonus)
  {
  }

  void set_lift_cbk(Lift_Cbk p_cbk, void* p_user_data = nullptr);

  void set_neighbor_cbk(Neighbor_Cbk p_cbk, void* p_user_data = nullptr);

  void set_lift_method(const std::string& p_method_name);

  void set_neighbor_method(const std::string& p_method_name);

  void score_lift(Lift_Ctx& p_ctx, size_t p_var_idx, double p_delta) const;

  void score_neighbor(Neighbor_Ctx& p_ctx,
                      size_t p_var_idx,
                      double p_delta) const;

private:
  Lift_Cbk m_lift_cbk;

  void* m_lift_user_data;

  Neighbor_Cbk m_neighbor_cbk;

  void* m_neighbor_user_data;

  enum class Lift_Method
  {
    lift_age,
    lift_random
  };

  enum class Neighbor_Method
  {
    progress_bonus,
    progress_age
  };

  Lift_Method m_lift_method;

  Neighbor_Method m_neighbor_method;

  void lift_age(Lift_Ctx& p_ctx, size_t p_var_idx, double p_delta) const;

  void
  lift_random(Lift_Ctx& p_ctx, size_t p_var_idx, double p_delta) const;

  void progress_bonus(Neighbor_Ctx& p_ctx,
                      size_t p_var_idx,
                      double p_delta) const;

  void progress_age(Neighbor_Ctx& p_ctx,
                    size_t p_var_idx,
                    double p_delta) const;
};