/*=====================================================================================

    Filename:     Local_MIP.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../local_search/Local_Search.h"
#include "../model_data/Model_Manager.h"
#include "../reader/Model_Reader.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class Local_MIP
{
private:
  std::string m_model_file;

  double m_time_limit;

  std::thread m_timeout_thread;

  std::mutex m_timeout_mutex;

  std::condition_variable m_timeout_cv;

  bool m_cancel_timeout;

  std::thread m_obj_log_thread;

  std::atomic<bool> m_stop_obj_log;

  bool m_log_obj_enabled;

  std::unique_ptr<Model_Reader> m_reader;

  std::unique_ptr<Model_Manager> m_model_manager;

  std::unique_ptr<Local_Search> m_local_search;

  void request_timeout_stop();

  void timeout_handler();

  void start_obj_logger();

  void stop_obj_logger();

  void obj_log_handler();

  void prepare_reader();

public:
  Local_MIP();

  ~Local_MIP();

  void set_model_file(const std::string& p_model_file);

  void set_time_limit(double p_time_limit);

  void set_bound_strengthen(int p_level);

  void set_split_eq(bool p_enable);

  void set_log_obj(bool p_enable);

  void set_sol_path(const std::string& p_sol_path);

  void set_random_seed(uint32_t p_seed);

  void set_feas_tolerance(double p_value);

  void set_opt_tolerance(double p_value);

  void set_zero_tolerance(double p_value);

  void set_start_method(const std::string& p_start_name);

  void set_start_cbk(Local_Search::Start_Cbk p_start_cbk,
                     void* p_user_data = nullptr);

  void set_restart_method(const std::string& p_restart_name);

  void set_restart_step(size_t p_restart_step);

  void set_restart_cbk(Local_Search::Restart_Cbk p_restart_cbk,
                       void* p_user_data = nullptr);

  void set_weight_method(const std::string& p_weight_name);

  void set_weight_cbk(Local_Search::Weight_Cbk p_weight_cbk,
                      void* p_user_data = nullptr);

  void set_weight_smooth_probability(size_t p_weight_smooth_prob);

  void set_lift_scoring_method(const std::string& p_method_name);

  void set_neighbor_scoring_method(const std::string& p_method_name);

  void set_lift_scoring_cbk(Local_Search::Lift_Scoring_Cbk p_cbk,
                            void* p_user_data = nullptr);

  void set_neighbor_scoring_cbk(Local_Search::Neighbor_Scoring_Cbk p_cbk,
                                void* p_user_data = nullptr);

  void terminate();

  void set_bms_unsat_con(size_t p_value);

  void set_bms_mtm_unsat_op(size_t p_value);

  void set_bms_sat_con(size_t p_value);

  void set_bms_mtm_sat_op(size_t p_value);

  void set_bms_flip_op(size_t p_value);

  void set_bms_easy_op(size_t p_value);

  void set_bms_random_op(size_t p_value);

  void clear_neighbor_list();

  void add_neighbor(const std::string& p_neighbor_name,
                    size_t p_bms_con,
                    size_t p_bms_op);

  void add_custom_neighbor(const std::string& p_neighbor_name,
                           Local_Search::Neighbor_Cbk p_neighbor_cbk,
                           void* p_user_data = nullptr);

  void reset_default_neighbor_list();

  void set_tabu_base(size_t p_value);

  void set_activity_period(size_t p_value);

  void set_tabu_variation(size_t p_value);

  void set_break_eq_feas(bool p_enable);

  void run();

  double get_obj_value() const;

  bool is_feasible() const;
};
