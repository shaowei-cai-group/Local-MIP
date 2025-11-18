/*=====================================================================================

    Filename:     context.h

    Description:  Read-only context for callback interfaces
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include <cstddef>
#include <vector>

class Model_Manager;

struct Readonly_Ctx
{
  Readonly_Ctx(const Model_Manager& p_model_manager,
               const std::vector<double>& p_var_current_value,
               const std::vector<double>& p_var_best_value,
               const std::vector<double>& p_con_activity,
               const std::vector<double>& p_con_constant,
               const std::vector<bool>& p_con_is_equality,
               const std::vector<size_t>& p_con_weight,
               const std::vector<size_t>& p_con_unsat_idxs,
               const std::vector<size_t>& p_con_pos_in_unsat_idxs,
               const std::vector<size_t>& p_con_sat_idxs,
               const std::vector<size_t>& p_var_last_dec_step,
               const std::vector<size_t>& p_var_last_inc_step,
               const std::vector<size_t>& p_var_allow_inc_step,
               const std::vector<size_t>& p_var_allow_dec_step,
               const size_t& p_obj_var_num,
               const std::vector<double>& p_var_obj_cost,
               const bool& p_is_found_feasible,
               const double& p_best_obj,
               const size_t& p_cur_step,
               const size_t& p_last_improve_step,
               const bool& p_current_obj_breakthrough,
               const std::vector<size_t>& p_binary_idx_list,
               const std::vector<size_t>& p_non_fixed_var_idx_list);

  const Model_Manager& m_model_manager;

  const std::vector<double>& m_var_current_value;

  const std::vector<double>& m_var_best_value;

  const std::vector<double>& m_con_activity;

  const std::vector<double>& m_con_constant;

  const std::vector<bool>& m_con_is_equality;

  const std::vector<size_t>& m_con_weight;

  const std::vector<size_t>& m_con_unsat_idxs;

  const std::vector<size_t>& m_con_pos_in_unsat_idxs;

  const std::vector<size_t>& m_con_sat_idxs;

  const std::vector<size_t>& m_var_last_dec_step;

  const std::vector<size_t>& m_var_last_inc_step;

  const std::vector<size_t>& m_var_allow_inc_step;

  const std::vector<size_t>& m_var_allow_dec_step;

  const size_t& m_obj_var_num;

  const std::vector<double>& m_var_obj_cost;

  const bool& m_is_found_feasible;

  const double& m_best_obj;

  const bool& m_current_obj_breakthrough;

  const size_t& m_cur_step;

  const size_t& m_last_improve_step;

  const std::vector<size_t>& m_binary_idx_list;

  const std::vector<size_t>& m_non_fixed_var_idx_list;
};

inline Readonly_Ctx::Readonly_Ctx(
    const Model_Manager& p_model_manager,
    const std::vector<double>& p_var_current_value,
    const std::vector<double>& p_var_best_value,
    const std::vector<double>& p_con_activity,
    const std::vector<double>& p_con_constant,
    const std::vector<bool>& p_con_is_equality,
    const std::vector<size_t>& p_con_weight,
    const std::vector<size_t>& p_con_unsat_idxs,
    const std::vector<size_t>& p_con_pos_in_unsat_idxs,
    const std::vector<size_t>& p_con_sat_idxs,
    const std::vector<size_t>& p_var_last_dec_step,
    const std::vector<size_t>& p_var_last_inc_step,
    const std::vector<size_t>& p_var_allow_inc_step,
    const std::vector<size_t>& p_var_allow_dec_step,
    const size_t& p_obj_var_num,
    const std::vector<double>& p_var_obj_cost,
    const bool& p_is_found_feasible,
    const double& p_best_obj,
    const size_t& p_cur_step,
    const size_t& p_last_improve_step,
    const bool& p_current_obj_breakthrough,
    const std::vector<size_t>& p_binary_idx_list,
    const std::vector<size_t>& p_non_fixed_var_idx_list)
    : m_model_manager(p_model_manager),
      m_var_current_value(p_var_current_value),
      m_var_best_value(p_var_best_value), m_con_activity(p_con_activity),
      m_con_constant(p_con_constant), m_con_is_equality(p_con_is_equality),
      m_con_weight(p_con_weight), m_con_unsat_idxs(p_con_unsat_idxs),
      m_con_pos_in_unsat_idxs(p_con_pos_in_unsat_idxs),
      m_con_sat_idxs(p_con_sat_idxs),
      m_var_last_dec_step(p_var_last_dec_step),
      m_var_last_inc_step(p_var_last_inc_step),
      m_var_allow_inc_step(p_var_allow_inc_step),
      m_var_allow_dec_step(p_var_allow_dec_step),
      m_obj_var_num(p_obj_var_num), m_var_obj_cost(p_var_obj_cost),
      m_is_found_feasible(p_is_found_feasible), m_best_obj(p_best_obj),
      m_current_obj_breakthrough(p_current_obj_breakthrough),
      m_cur_step(p_cur_step), m_last_improve_step(p_last_improve_step),
      m_binary_idx_list(p_binary_idx_list),
      m_non_fixed_var_idx_list(p_non_fixed_var_idx_list)
{
}