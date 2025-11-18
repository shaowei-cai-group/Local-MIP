/*=====================================================================================

    Filename:     start.h

    Description:  Initial solution generation strategies interface
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

class Start
{
public:
  class Start_Ctx
  {
  public:
    Start_Ctx(const Readonly_Ctx& p_shared,
              std::vector<double>& p_var_values,
              std::mt19937& p_rng);

    Start_Ctx(const Start_Ctx&) = delete;

    Start_Ctx& operator=(const Start_Ctx&) = delete;

    const Readonly_Ctx& m_shared;

    std::vector<double>& m_var_current_value;

    std::mt19937& m_rng;
  };

  using Start_Cbk = std::function<void(Start_Ctx&, void*)>;

  Start();

  void set_cbk(Start_Cbk p_start_cbk, void* p_user_data = nullptr);

  void set_method(const std::string& p_method_name);

  void set_up_start_values(Start_Ctx& p_ctx) const;

private:
  enum class Method
  {
    zero,
    random
  };

  Start_Cbk m_user_cbk;

  void* m_user_data;

  Method m_default_method;

  void zero_start(Start_Ctx& p_ctx) const;

  void random_start(Start_Ctx& p_ctx) const;
};
