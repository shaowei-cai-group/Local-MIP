#include "../src/local_mip/Local_MIP.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// RAII helper to manage GIL for callbacks
struct PyCallbackGuard
{
  py::gil_scoped_acquire lock;
  PyCallbackGuard() = default;
};

PYBIND11_MODULE(localmip_py, m)
{
  m.doc() = "Python bindings for Local-MIP (minimal surface)";

  py::enum_<Model_API::Sense>(m, "Sense")
      .value("minimize", Model_API::Sense::minimize)
      .value("maximize", Model_API::Sense::maximize);

  py::enum_<Var_Type>(m, "VarType")
      .value("binary", Var_Type::binary)
      .value("general_integer", Var_Type::general_integer)
      .value("real", Var_Type::real)
      .value("fixed", Var_Type::fixed);

  py::class_<Local_MIP>(m, "LocalMIP")
      .def(py::init<>())

      // Basic configuration
      .def("set_model_file", &Local_MIP::set_model_file, py::arg("path"))
      .def(
          "set_time_limit", &Local_MIP::set_time_limit, py::arg("seconds"))
      .def("set_sol_path", &Local_MIP::set_sol_path, py::arg("path"))
      .def("set_log_obj", &Local_MIP::set_log_obj, py::arg("enable"))
      .def("set_bound_strengthen",
           &Local_MIP::set_bound_strengthen,
           py::arg("level"))
      .def("set_split_eq", &Local_MIP::set_split_eq, py::arg("enable"))
      .def("set_random_seed", &Local_MIP::set_random_seed, py::arg("seed"))
      .def("set_feas_tolerance",
           &Local_MIP::set_feas_tolerance,
           py::arg("value"))
      .def("set_opt_tolerance",
           &Local_MIP::set_opt_tolerance,
           py::arg("value"))
      .def("set_zero_tolerance",
           &Local_MIP::set_zero_tolerance,
           py::arg("value"))

      // Heuristic selection
      .def("set_start_method",
           &Local_MIP::set_start_method,
           py::arg("method_name"))
      .def("set_restart_method",
           &Local_MIP::set_restart_method,
           py::arg("method_name"))
      .def("set_restart_step",
           &Local_MIP::set_restart_step,
           py::arg("step"))
      .def("set_weight_method",
           &Local_MIP::set_weight_method,
           py::arg("method_name"))
      .def("set_weight_smooth_probability",
           &Local_MIP::set_weight_smooth_probability,
           py::arg("prob"))
      .def("set_lift_scoring_method",
           &Local_MIP::set_lift_scoring_method,
           py::arg("method_name"))
      .def("set_neighbor_scoring_method",
           &Local_MIP::set_neighbor_scoring_method,
           py::arg("method_name"))

      // Neighbor list config
      .def("clear_neighbor_list", &Local_MIP::clear_neighbor_list)
      .def("add_neighbor",
           &Local_MIP::add_neighbor,
           py::arg("name"),
           py::arg("bms_con"),
           py::arg("bms_op"))
      .def(
          "add_custom_neighbor",
          [](Local_MIP& self, const std::string& name, py::function fn)
          {
            auto cbk =
                [fn](Neighbor::Neighbor_Ctx& ctx, void* user_data) mutable
            {
              (void)user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx));
            };
            self.add_custom_neighbor(name, cbk, nullptr);
          },
          py::arg("name"),
          py::arg("callable"))
      .def("reset_default_neighbor_list",
           &Local_MIP::reset_default_neighbor_list)

      // Tabu / activity
      .def("set_tabu_base", &Local_MIP::set_tabu_base, py::arg("value"))
      .def("set_activity_period",
           &Local_MIP::set_activity_period,
           py::arg("value"))
      .def("set_tabu_variation",
           &Local_MIP::set_tabu_variation,
           py::arg("value"))
      .def("set_break_eq_feas",
           &Local_MIP::set_break_eq_feas,
           py::arg("enable"))

      // Model API (programmatic model building)
      .def("enable_model_api", &Local_MIP::enable_model_api)
      .def("set_sense", &Local_MIP::set_sense, py::arg("sense"))
      .def("set_obj_offset", &Local_MIP::set_obj_offset, py::arg("offset"))
      .def("add_var",
           &Local_MIP::add_var,
           py::arg("name"),
           py::arg("lb"),
           py::arg("ub"),
           py::arg("cost") = 0.0,
           py::arg("type") = Var_Type::real)
      .def("set_cost",
           py::overload_cast<int, double>(&Local_MIP::set_cost),
           py::arg("col"),
           py::arg("cost"))
      .def("set_cost",
           py::overload_cast<const std::string&, double>(
               &Local_MIP::set_cost),
           py::arg("name"),
           py::arg("cost"))
      .def("add_con",
           py::overload_cast<double,
                             double,
                             const std::vector<int>&,
                             const std::vector<double>&>(
               &Local_MIP::add_con),
           py::arg("lb"),
           py::arg("ub"),
           py::arg("cols"),
           py::arg("coefs"))
      .def("add_con",
           py::overload_cast<double,
                             double,
                             const std::vector<std::string>&,
                             const std::vector<double>&>(
               &Local_MIP::add_con),
           py::arg("lb"),
           py::arg("ub"),
           py::arg("names"),
           py::arg("coefs"))
      .def("add_var_to_con",
           py::overload_cast<int, int, double>(&Local_MIP::add_var_to_con),
           py::arg("row"),
           py::arg("col"),
           py::arg("coef"))
      .def("add_var_to_con",
           py::overload_cast<int, const std::string&, double>(
               &Local_MIP::add_var_to_con),
           py::arg("row"),
           py::arg("name"),
           py::arg("coef"))
      .def("set_integrality",
           py::overload_cast<int, Var_Type>(&Local_MIP::set_integrality),
           py::arg("col"),
           py::arg("type"))
      .def("set_integrality",
           py::overload_cast<const std::string&, Var_Type>(
               &Local_MIP::set_integrality),
           py::arg("name"),
           py::arg("type"))

      // Callbacks
      .def(
          "set_start_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto cbk =
                [fn](Start::Start_Ctx& ctx, void* p_user_data) mutable
            {
              (void)p_user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx));
            };
            self.set_start_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_restart_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto cbk =
                [fn](Restart::Restart_Ctx& ctx, void* p_user_data) mutable
            {
              (void)p_user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx));
            };
            self.set_restart_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_weight_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto cbk =
                [fn](Weight::Weight_Ctx& ctx, void* p_user_data) mutable
            {
              (void)p_user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx));
            };
            self.set_weight_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_lift_scoring_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto cbk = [fn](Scoring::Lift_Ctx& ctx,
                            size_t var_idx,
                            double delta,
                            void* p_user_data) mutable
            {
              (void)p_user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx), var_idx, delta);
            };
            self.set_lift_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_neighbor_scoring_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto cbk = [fn](Scoring::Neighbor_Ctx& ctx,
                            size_t var_idx,
                            double delta,
                            void* p_user_data) mutable
            {
              (void)p_user_data;
              PyCallbackGuard g;
              fn(py::capsule(&ctx), var_idx, delta);
            };
            self.set_neighbor_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"))

      // Execution
      .def("run",
           [](Local_MIP& self)
           {
             py::gil_scoped_release release;
             self.run();
           })
      .def("terminate", &Local_MIP::terminate)

      // Results
      .def("get_obj_value", &Local_MIP::get_obj_value)
      .def("is_feasible", &Local_MIP::is_feasible)
      .def("get_solution",
           &Local_MIP::get_solution,
           py::return_value_policy::reference_internal);
}
