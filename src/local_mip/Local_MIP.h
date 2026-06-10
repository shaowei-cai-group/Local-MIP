/*=====================================================================================

    Filename:     Local_MIP.h

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once
#include "../local_search/Local_Search.h"
#include "../model_api/Model_API.h"
#include "../model_data/Model_Manager.h"
#include "../reader/Model_Reader.h"
#include "utils/global_defs.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Local_MIP
{
private:
  std::string m_model_file;

  std::string m_param_set_file;

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

  std::unique_ptr<Model_API> m_model_api;

  bool m_use_model_api;

  bool m_model_loaded;

  void request_timeout_stop();

  void timeout_handler();

  void start_obj_logger();

  void stop_obj_logger();

  void obj_log_handler();

  void prepare_reader();

  bool check_model_api() const;

public:
  Local_MIP();

  ~Local_MIP();

  void set_model_file(const std::string& p_model_file);

  void set_param_set_file(const std::string& p_param_set_file);

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

  void enable_model_api();

  void set_sense(Model_API::Sense p_sense);

  bool set_obj_offset(double p_offset);

  int add_var(const std::string& p_name,
              double p_lb,
              double p_ub,
              double p_cost = 0.0,
              Var_Type p_type = Var_Type::real);

  bool set_cost(int p_col, double p_cost);

  bool set_cost(const std::string& p_name, double p_cost);

  int add_con(double p_lb,
              double p_ub,
              const std::vector<int>& p_cols,
              const std::vector<double>& p_coefs);

  int add_con(double p_lb,
              double p_ub,
              const std::vector<std::string>& p_names,
              const std::vector<double>& p_coefs);

  bool add_var_to_con(int p_row, int p_col, double p_coef);

  bool add_var_to_con(int p_row, const std::string& p_name, double p_coef);

  bool set_integrality(int p_col, Var_Type p_type);

  bool set_integrality(const std::string& p_name, Var_Type p_type);

  void load_model();

  void run();

  double get_obj_value() const;

  bool is_feasible() const;

  const std::vector<double>& get_solution() const;

  const Model_Manager* get_model_manager() const;

  using Improvement_Cbk =
      std::function<void(const double* p_sol, size_t p_var_num, double p_obj)>;

  using Exchange_Check_Cbk = std::function<void()>;

  using Infeas_Improvement_Cbk = std::function<void(
      const double* p_sol, size_t p_var_num, size_t p_unsat_num)>;

  // Called from the search thread when a new feasible incumbent is accepted.
  // p_sol is valid only for the duration of the callback; copy it if it must
  // outlive the call. p_obj is reported in user objective sense.
  void set_on_improvement_callback(Improvement_Cbk p_cbk);

  // Inject an externally found feasible incumbent. The solution must match the
  // model variable order and satisfy bounds, integrality, and constraints.
  // Returns false for invalid, infeasible, or non-improving incumbents.
  bool inject_solution(const double* p_sol, size_t p_var_num, double p_obj);

  // Replace the current search point and reset local-search state. This API is
  // intended to be called from the search thread, typically inside the exchange
  // callback. The injected point may be infeasible but must satisfy variable
  // bounds and integrality.
  bool inject_to_current_and_restart(const double* p_sol,
                                     size_t p_var_num,
                                     size_t p_restart_step_override);

  // Set how often the search loop invokes the exchange callback. A zero
  // interval disables periodic checks.
  void set_exchange_check_interval(size_t p_interval);

  // Called from the search thread every p_interval search steps. The callback
  // may call injection APIs directly; it must not mutate Local_MIP from a
  // separate thread unless external synchronization is provided.
  void set_exchange_check_callback(Exchange_Check_Cbk p_cbk);

  // Called before the first feasible solution whenever the current solution
  // improves the best known number of unsatisfied constraints.
  void set_on_infeas_improvement_callback(Infeas_Improvement_Cbk p_cbk);

  // Update the best known infeasible progress without changing the current
  // search point.
  bool inject_infeas_bound(size_t p_var_num, size_t p_unsat_num);

  // Optionally inject an infeasible current point together with a better unsat
  // count. If p_sol is nullptr, this is equivalent to inject_infeas_bound().
  bool inject_infeas_solution(const double* p_sol,
                              size_t p_var_num,
                              size_t p_unsat_num);
};
