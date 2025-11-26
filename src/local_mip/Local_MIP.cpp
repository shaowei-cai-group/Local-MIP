/*=====================================================================================

    Filename:     Local_MIP.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../local_search/Local_Search.h"
#include "../model_data/Model_Manager.h"
#include "../reader/LP_Reader.h"
#include "../reader/MPS_Reader.h"
#include "../utils/global_defs.h"
#include "../utils/solver_error.h"
#include "Local_MIP.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

Local_MIP::Local_MIP()
    : m_model_file(""), m_time_limit(10.0), m_timeout_thread(),
      m_timeout_mutex(), m_timeout_cv(), m_cancel_timeout(true),
      m_obj_log_thread(), m_stop_obj_log(true), m_log_obj_enabled(true),
      m_reader(nullptr),
      m_model_manager(std::make_unique<Model_Manager>()),
      m_local_search(std::make_unique<Local_Search>(m_model_manager.get()))
{
}

Local_MIP::~Local_MIP()
{
  stop_obj_logger();
  request_timeout_stop();
  if (m_timeout_thread.joinable())
    m_timeout_thread.join();
}

void Local_MIP::set_model_file(const std::string& p_model_file)
{
  m_model_file = p_model_file;
  printf("c model file is set to : %s\n", m_model_file.c_str());
}

void Local_MIP::set_time_limit(double p_time_limit)
{
  if (p_time_limit <= 0.0)
    throw std::invalid_argument("time limit must be positive");
  m_time_limit = p_time_limit;
  printf("c time limit is set to : %.2lf seconds\n", m_time_limit);
}

void Local_MIP::set_bound_strengthen(int p_level)
{
  m_model_manager->set_bound_strengthen(p_level);
  printf("c bound strengthen level is set to : %d\n", p_level);
}

void Local_MIP::set_split_eq(bool p_enable)
{
  m_model_manager->set_split_eq(p_enable);
  printf("c split equality conversion is set to : %s\n",
         p_enable ? "true" : "false");
}

void Local_MIP::set_log_obj(bool p_enable)
{
  m_log_obj_enabled = p_enable;
  printf("c log obj is set to : %s\n", p_enable ? "true" : "false");
}

void Local_MIP::set_sol_path(const std::string& p_sol_path)
{
  m_local_search->set_sol_path(p_sol_path);
  printf("c sol path is set to : %s\n", p_sol_path.c_str());
}

void Local_MIP::set_random_seed(uint32_t p_seed)
{
  m_local_search->set_random_seed(p_seed);
  printf("c random seed is set to : %u%s\n",
         p_seed,
         p_seed == 0 ? " (use default internal seed)" : "");
}

void Local_MIP::set_feas_tolerance(double p_value)
{
  k_feas_tolerance = p_value;
  printf("c feasibility tolerance is set to : %.10g\n", p_value);
}

void Local_MIP::set_opt_tolerance(double p_value)
{
  k_opt_tolerance = p_value;
  printf("c optimality tolerance is set to : %.10g\n", p_value);
}

void Local_MIP::set_zero_tolerance(double p_value)
{
  k_zero_tolerance = p_value;
  printf("c zero value tolerance is set to : %.10g\n", p_value);
}

void Local_MIP::set_start_method(const std::string& p_method_name)
{
  printf("c init method is set to : %s\n", p_method_name.c_str());
  m_local_search->set_start_method(p_method_name);
}

void Local_MIP::set_start_cbk(Local_Search::Start_Cbk p_start_cbk,
                              void* p_user_data)
{
  m_local_search->set_start_cbk(std::move(p_start_cbk), p_user_data);
  printf("c custom start callback is registered.\n");
}

void Local_MIP::set_restart_method(const std::string& p_restart_name)
{
  printf("c restart method is set to : %s\n", p_restart_name.c_str());
  m_local_search->set_restart_method(p_restart_name);
}

void Local_MIP::set_restart_step(size_t p_restart_step)
{
  printf("c restart step is set to : %zu\n", p_restart_step);
  m_local_search->set_restart_step(p_restart_step);
}

void Local_MIP::set_restart_cbk(Local_Search::Restart_Cbk p_restart_cbk,
                                void* p_user_data)
{
  m_local_search->set_restart_cbk(std::move(p_restart_cbk), p_user_data);
  printf("c custom restart callback is registered.\n");
}

void Local_MIP::set_weight_method(const std::string& p_weight_name)
{
  printf("c weight method is set to : %s\n", p_weight_name.c_str());
  m_local_search->set_weight_method(p_weight_name);
}

void Local_MIP::set_weight_cbk(Local_Search::Weight_Cbk p_weight_cbk,
                               void* p_user_data)
{
  m_local_search->set_weight_cbk(std::move(p_weight_cbk), p_user_data);
  printf("c custom weight callback is registered.\n");
}

void Local_MIP::set_weight_smooth_probability(size_t p_weight_smooth_prob)
{
  printf("c weight smooth probability is set to : %zu\n",
         p_weight_smooth_prob);
  m_local_search->set_weight_smooth_probability(p_weight_smooth_prob);
}

void Local_MIP::set_lift_scoring_method(const std::string& p_method_name)
{
  printf("c lift scoring method is set to : %s\n", p_method_name.c_str());
  m_local_search->set_lift_scoring_method(p_method_name);
}

void Local_MIP::set_neighbor_scoring_method(
    const std::string& p_method_name)
{
  printf("c neighbor scoring method is set to : %s\n",
         p_method_name.c_str());
  m_local_search->set_neighbor_scoring_method(p_method_name);
}

void Local_MIP::set_lift_scoring_cbk(Local_Search::Lift_Scoring_Cbk p_cbk,
                                     void* p_user_data)
{
  m_local_search->set_lift_scoring_cbk(std::move(p_cbk), p_user_data);
  printf("c custom lift scoring callback is registered.\n");
}

void Local_MIP::set_neighbor_scoring_cbk(
    Local_Search::Neighbor_Scoring_Cbk p_cbk,
    void* p_user_data)
{
  m_local_search->set_neighbor_scoring_cbk(std::move(p_cbk), p_user_data);
  printf("c custom neighbor scoring callback is registered.\n");
}

void Local_MIP::set_bms_unsat_con(size_t p_value)
{
  m_local_search->set_bms_unsat_con(p_value);
  printf("c unsatisfied constraint sample size : %zu\n", p_value);
}

void Local_MIP::set_bms_mtm_unsat_op(size_t p_value)
{
  m_local_search->set_bms_mtm_unsat_op(p_value);
  printf("c unsatisfied MTM operations: %zu\n", p_value);
}

void Local_MIP::set_bms_sat_con(size_t p_value)
{
  m_local_search->set_bms_sat_con(p_value);
  printf("c satisfied constraint sample size : %zu\n", p_value);
}

void Local_MIP::set_bms_mtm_sat_op(size_t p_value)
{
  m_local_search->set_bms_mtm_sat_op(p_value);
  printf("c satisfied MTM operations : %zu\n", p_value);
}

void Local_MIP::set_bms_flip_op(size_t p_value)
{
  m_local_search->set_bms_flip_op(p_value);
  printf("c flip operations : %zu\n", p_value);
}

void Local_MIP::set_bms_easy_op(size_t p_value)
{
  m_local_search->set_bms_easy_op(p_value);
  printf("c easy operations : %zu\n", p_value);
}

void Local_MIP::set_bms_random_op(size_t p_value)
{
  m_local_search->set_bms_random_op(p_value);
  printf("c random unsatisfied operations : %zu\n", p_value);
}

void Local_MIP::clear_neighbor_list()
{
  m_local_search->clear_neighbor_list();
  printf("c neighbor list cleared\n");
}

void Local_MIP::add_neighbor(const std::string& p_neighbor_name,
                             size_t p_bms_con,
                             size_t p_bms_op)
{
  m_local_search->add_neighbor(p_neighbor_name, p_bms_con, p_bms_op);
  printf("c added neighbor: %s (bms_con=%zu, bms_op=%zu)\n",
         p_neighbor_name.c_str(),
         p_bms_con,
         p_bms_op);
}

void Local_MIP::add_custom_neighbor(
    const std::string& p_neighbor_name,
    Local_Search::Neighbor_Cbk p_neighbor_cbk,
    void* p_user_data)
{
  m_local_search->add_custom_neighbor(
      p_neighbor_name, std::move(p_neighbor_cbk), p_user_data);
  printf("c added custom neighbor: %s\n", p_neighbor_name.c_str());
}

void Local_MIP::reset_default_neighbor_list()
{
  m_local_search->reset_default_neighbor_list();
  printf("c neighbor list reset to default\n");
}

void Local_MIP::set_tabu_base(size_t p_value)
{
  m_local_search->set_tabu_base(p_value);
  printf("c tabu tenure base : %zu\n", p_value);
}

void Local_MIP::set_activity_period(size_t p_value)
{
  m_local_search->set_activity_period(p_value);
  printf("c constraint activity period : %zu\n", p_value);
}

void Local_MIP::set_tabu_variation(size_t p_value)
{
  m_local_search->set_tabu_variation(p_value);
  printf("c tabu tenure variation : %zu\n", p_value);
}

void Local_MIP::set_break_eq_feas(bool p_enable)
{
  m_local_search->set_break_eq_feas(p_enable);
  printf("c break feasibility on equality constraints is set to : %s\n",
         p_enable ? "true" : "false");
}

void Local_MIP::run()
{
  prepare_reader();
  m_reader->read(m_model_file.c_str());
  if (!m_model_manager->process_after_read())
  {
    printf("c model is infeasible, skip local search.\n");
    return;
  }
  g_clk_start = std::chrono::steady_clock::now();
  m_cancel_timeout = false;
  m_timeout_thread = std::thread(&Local_MIP::timeout_handler, this);
  start_obj_logger();
  m_local_search->run_search();
  stop_obj_logger();
  request_timeout_stop();
  if (m_timeout_thread.joinable())
    m_timeout_thread.join();
  m_local_search->output_result();
  printf("c [%10.2lf] local search is finished.\n", elapsed_time());
}

void Local_MIP::terminate()
{
  m_local_search->terminate();
  stop_obj_logger();
  request_timeout_stop();
  printf("c [%10.2lf] local search is terminated by user.\n",
         elapsed_time());
}

void Local_MIP::timeout_handler()
{
  {
    std::unique_lock<std::mutex> lock(m_timeout_mutex);
    bool cancelled =
        m_timeout_cv.wait_for(lock,
                              std::chrono::duration<double>(m_time_limit),
                              [this]() { return m_cancel_timeout; });
    if (cancelled)
      return;
  }
  m_local_search->terminate();
  printf("c [%10.2lf] local search is terminated by timeout.\n",
         elapsed_time());
}

void Local_MIP::request_timeout_stop()
{
  {
    std::lock_guard<std::mutex> lock(m_timeout_mutex);
    m_cancel_timeout = true;
  }
  m_timeout_cv.notify_all();
}

void Local_MIP::start_obj_logger()
{
  if (!m_log_obj_enabled)
    return;
  stop_obj_logger();
  m_stop_obj_log.store(false, std::memory_order_relaxed);
  m_obj_log_thread = std::thread(&Local_MIP::obj_log_handler, this);
}

void Local_MIP::stop_obj_logger()
{
  m_stop_obj_log.store(true, std::memory_order_relaxed);
  if (m_obj_log_thread.joinable())
    m_obj_log_thread.join();
}

void Local_MIP::obj_log_handler()
{
  double last_value = std::numeric_limits<double>::quiet_NaN();
  bool has_value = false;
  while (true)
  {
    double current_value = m_local_search->get_obj_value();
    if (!std::isnan(current_value) &&
        (!has_value || current_value != last_value))
    {
      last_value = current_value;
      has_value = true;
      printf(
          "c [%10.2lf] obj*: %-20.15g\n", elapsed_time(), current_value);
    }
    if (m_stop_obj_log.load(std::memory_order_relaxed))
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void Local_MIP::prepare_reader()
{
  if (m_model_file.empty())
    throw Solver_Error(
        "model file path is empty, call set_model_file() first");
  m_reader.reset();
  std::string extension;
  auto dot_pos = m_model_file.find_last_of('.');
  if (dot_pos != std::string::npos)
    extension = m_model_file.substr(dot_pos + 1);
  std::transform(extension.begin(),
                 extension.end(),
                 extension.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::tolower(ch)); });
  if (extension == "mps")
    m_reader = std::make_unique<MPS_Reader>(m_model_manager.get());
  else if (extension == "lp")
    m_reader = std::make_unique<LP_Reader>(m_model_manager.get());
  else
  {
    std::string message = "unsupported model file format: " + m_model_file;
    printf("o %s\n", message.c_str());
    throw Solver_Error(message);
  }
}

double Local_MIP::get_obj_value() const
{
  return m_local_search->get_obj_value();
}

bool Local_MIP::is_feasible() const
{
  return m_local_search->is_feasible();
}

const std::vector<double>& Local_MIP::get_solution() const
{
  return m_local_search->get_solution();
}

const Model_Manager* Local_MIP::get_model_manager() const
{
  return m_model_manager.get();
}
