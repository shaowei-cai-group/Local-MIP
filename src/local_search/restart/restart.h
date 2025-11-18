/*=====================================================================================

    Filename:     restart.h

    Description:  Search restart mechanisms interface
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include "../../model_data/Model_Manager.h"
#include "../context/context.h"
#include <functional>
#include <random>
#include <string>
#include <vector>

class Restart
{
public:
  class Restart_Ctx
  {
  public:
    Restart_Ctx(const Readonly_Ctx& p_shared,
                std::vector<double>& p_current_values,
                std::mt19937& p_rng,
                std::vector<size_t>& p_con_weight);

    Restart_Ctx(const Restart_Ctx&) = delete;

    Restart_Ctx& operator=(const Restart_Ctx&) = delete;

    const Readonly_Ctx& m_shared;

    std::vector<double>& m_var_current_value;

    std::mt19937& m_rng;

    std::vector<size_t>& m_con_weight;
  };

  using Restart_Cbk = std::function<void(Restart_Ctx&, void*)>;

  Restart();

  void set_cbk(Restart_Cbk p_restart_cbk, void* p_user_data = nullptr);

  void set_method(const std::string& p_restart_name);

  void set_restart_step(size_t p_restart_step);

  bool execute(Restart_Ctx& p_ctx) const;

private:
  enum class Strategy
  {
    random,
    best,
    hybrid
  };

  Restart_Cbk m_user_cbk;

  void* m_user_data;

  Strategy m_default_strategy;

  size_t m_restart_step;

  inline bool should_restart(Restart_Ctx& p_ctx) const;

  void reset_weights(Restart_Ctx& p_ctx) const;

  void random_restart(Restart_Ctx& p_ctx) const;

  void best_restart(Restart_Ctx& p_ctx) const;

  void hybrid_restart(Restart_Ctx& p_ctx) const;

  double sample_random_value(Restart_Ctx& p_ctx,
                             const Model_Var& p_model_var) const;
};

inline bool Restart::should_restart(Restart_Ctx& p_ctx) const
{
  if (m_restart_step == 0)
    return false;
  return p_ctx.m_shared.m_cur_step >
         p_ctx.m_shared.m_last_improve_step + m_restart_step;
}
