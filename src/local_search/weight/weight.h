/*=====================================================================================

    Filename:     weight.h

    Description:  Weight update strategies interface
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include "../../model_data/Model_Manager.h"
#include "../context/context.h"
#include <cstddef>
#include <functional>
#include <random>
#include <string>
#include <vector>

class Weight
{
public:
  class Weight_Ctx
  {
  public:
    Weight_Ctx(const Readonly_Ctx& p_shared,
               std::mt19937& p_rng,
               std::vector<size_t>& p_con_weight);

    Weight_Ctx(const Weight_Ctx&) = delete;

    Weight_Ctx& operator=(const Weight_Ctx&) = delete;

    const Readonly_Ctx& m_shared;

    std::mt19937& m_rng;

    std::vector<size_t>& m_con_weight;
  };

  using Weight_Cbk = std::function<void(Weight_Ctx&, void*)>;

  Weight();

  void set_cbk(Weight_Cbk p_weight_cbk, void* p_user_data = nullptr);

  void set_method(const std::string& p_method_name);

  void set_smooth_probability(size_t p_weight_smooth_prob);

  const size_t& smooth_probability() const;

  void update(Weight_Ctx& p_ctx) const;

private:
  enum class Method
  {
    smooth,
    monotone
  };

  Weight_Cbk m_user_cbk;

  void* m_user_data;

  Method m_default_method;

  size_t m_smooth_prob;

  void smooth_update(Weight_Ctx& p_ctx) const;

  void monotone_update(Weight_Ctx& p_ctx) const;
};
