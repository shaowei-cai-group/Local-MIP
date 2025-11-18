/*=====================================================================================

    Filename:     neighbor.h

    Description:  Search neighbor mechanisms interface
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
#include <unordered_map>
#include <vector>

class Neighbor
{
public:
  class Neighbor_Ctx
  {
  public:
    Neighbor_Ctx(const Readonly_Ctx& p_shared,
                 std::vector<size_t>& p_op_var_idxs,
                 std::vector<double>& p_op_var_deltas,
                 size_t& p_op_size,
                 std::mt19937& p_rng);

    Neighbor_Ctx(const Neighbor_Ctx&) = delete;

    Neighbor_Ctx& operator=(const Neighbor_Ctx&) = delete;

    const Readonly_Ctx& m_shared;

    std::vector<size_t>& m_op_var_idxs;

    std::vector<double>& m_op_var_deltas;

    size_t& m_op_size;

    std::mt19937& m_rng;
  };

  using Neighbor_Cbk = std::function<void(Neighbor_Ctx&, void*)>;

  Neighbor(const std::string& p_neighbor_name,
           size_t p_bms_con,
           size_t p_bms_op);

  Neighbor(const std::string& p_neighbor_name,
           Neighbor_Cbk p_neighbor_cbk,
           void* p_user_data = nullptr);

  void set_cbk(Neighbor_Cbk p_neighbor_cbk, void* p_user_data = nullptr);

  void explore(Neighbor_Ctx& p_ctx);

private:
  enum class Strategy
  {
    unsat_mtm_bm,
    sat_mtm,
    flip,
    easy,
    unsat_mtm_bm_random,
    user_defined,
  };

  Strategy m_strategy;

  Neighbor_Cbk m_neighbor_cbk;

  void* m_user_data;

  size_t m_bms_con;

  size_t m_bms_op;

  static std::vector<size_t> m_bms_idxs;

  static std::unordered_map<size_t, size_t> m_remap;

  void explore_unsat_mtm_bm(Neighbor_Ctx& p_ctx);

  void explore_sat_mtm(Neighbor_Ctx& p_ctx);

  void explore_flip(Neighbor_Ctx& p_ctx);

  void explore_easy(Neighbor_Ctx& p_ctx);

  void explore_unsat_random_bm(Neighbor_Ctx& p_ctx);

  static inline const std::vector<size_t>&
  sample_idxs(const std::vector<size_t>& p_source_idxs,
              size_t p_max_sample,
              size_t& p_final_size,
              Neighbor_Ctx& p_ctx);

  static double inequality_mixed_tight_operation(size_t p_con_idx,
                                                 size_t p_term_idx,
                                                 size_t p_var_idx,
                                                 Neighbor_Ctx& p_ctx);

  static double equality_mixed_tight_operation(size_t p_con_idx,
                                               size_t p_term_idx,
                                               size_t p_var_idx,
                                               Neighbor_Ctx& p_ctx);

  static double breakthrough_operation(size_t p_term_idx,
                                       size_t p_var_idx,
                                       Neighbor_Ctx& p_ctx);

  static size_t sample_op(size_t p_max_ops,
                          std::vector<size_t>& p_op_var_idxs,
                          std::vector<double>& p_op_var_deltas,
                          Neighbor_Ctx& p_ctx);

  static bool tabu(Neighbor_Ctx& p_ctx, size_t p_var_idx, double p_delta);

  static bool
  tabu_latest(Neighbor_Ctx& p_ctx, size_t p_var_idx, double p_delta);
};

inline size_t Neighbor::sample_op(size_t p_max_ops,
                                  std::vector<size_t>& p_op_var_idxs,
                                  std::vector<double>& p_op_var_deltas,
                                  Neighbor_Ctx& p_ctx)
{
  size_t available = p_op_var_idxs.size();
  if (available == 0 || p_max_ops == 0)
    return 0;
  if (available <= p_max_ops)
    return available;
  for (size_t base_idx = 0; base_idx < p_max_ops; ++base_idx)
  {
    size_t remaining = available - base_idx;
    std::uniform_int_distribution<size_t> dist(0, remaining - 1);
    size_t random_idx = dist(p_ctx.m_rng) + base_idx;
    std::swap(p_op_var_idxs[random_idx], p_op_var_idxs[base_idx]);
    std::swap(p_op_var_deltas[random_idx], p_op_var_deltas[base_idx]);
  }
  return p_max_ops;
}

inline const std::vector<size_t>&
Neighbor::sample_idxs(const std::vector<size_t>& p_source_idxs,
                      size_t p_max_sample,
                      size_t& p_final_size,
                      Neighbor_Ctx& p_ctx)
{
  p_final_size = p_source_idxs.size();
  if (p_final_size <= p_max_sample)
    return p_source_idxs;
  p_final_size = p_max_sample;
  m_bms_idxs.clear();
  m_remap.clear();
  size_t available = p_source_idxs.size();
  for (size_t sample_idx = 0; sample_idx < p_max_sample; ++sample_idx)
  {
    std::uniform_int_distribution<size_t> dist(0, available - 1);
    size_t random_idx = dist(p_ctx.m_rng);
    size_t actual_idx = random_idx;
    auto it = m_remap.find(random_idx);
    if (it != m_remap.end())
      actual_idx = it->second;
    size_t last_idx = available - 1;
    size_t mapped_last = last_idx;
    auto last_it = m_remap.find(last_idx);
    if (last_it != m_remap.end())
      mapped_last = last_it->second;
    m_remap[random_idx] = mapped_last;
    m_bms_idxs.push_back(p_source_idxs[actual_idx]);
    --available;
  }
  return m_bms_idxs;
}

inline bool
Neighbor::tabu(Neighbor_Ctx& p_ctx, size_t p_var_idx, double p_delta)
{
  if (p_delta < 0 && p_ctx.m_shared.m_cur_step <
                         p_ctx.m_shared.m_var_allow_dec_step[p_var_idx])
    return true;
  if (p_delta > 0 && p_ctx.m_shared.m_cur_step <
                         p_ctx.m_shared.m_var_allow_inc_step[p_var_idx])
    return true;
  return false;
}

inline bool Neighbor::tabu_latest(Neighbor_Ctx& p_ctx,
                                  size_t p_var_idx,
                                  double p_delta)
{
  if (p_delta < 0 && p_ctx.m_shared.m_cur_step ==
                         p_ctx.m_shared.m_var_last_inc_step[p_var_idx] + 1)
    return true;
  if (p_delta > 0 && p_ctx.m_shared.m_cur_step ==
                         p_ctx.m_shared.m_var_last_dec_step[p_var_idx] + 1)
    return true;
  return false;
}
