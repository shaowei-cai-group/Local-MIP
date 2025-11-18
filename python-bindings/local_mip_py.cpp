#include <cstdint>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../src/local_mip/Local_MIP.h"

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

  py::class_<Local_MIP>(m, "LocalMIP")
      .def(py::init<>())

      // Basic configuration
      .def("set_model_file", &Local_MIP::set_model_file, py::arg("path"))
      .def("set_time_limit", &Local_MIP::set_time_limit, py::arg("seconds"))
      .def("set_sol_path", &Local_MIP::set_sol_path, py::arg("path"))
      .def("set_log_obj", &Local_MIP::set_log_obj, py::arg("enable"))
      .def("set_bound_strengthen",
           &Local_MIP::set_bound_strengthen,
           py::arg("level"))
      .def("set_split_eq", &Local_MIP::set_split_eq, py::arg("enable"))

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
      .def("add_custom_neighbor",
           [](Local_MIP& self, const std::string& name, py::function fn)
           {
             auto cbk = [fn](Neighbor::Neighbor_Ctx& ctx, void* user_data)
             {
               (void)user_data;
               PyCallbackGuard g;
               fn(py::capsule(&ctx));
             };
             self.add_custom_neighbor(name, cbk, nullptr);
           },
           py::arg("name"),
           py::arg("callable"))
      .def("reset_default_neighbor_list", &Local_MIP::reset_default_neighbor_list)

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

      // Callbacks
      .def("set_start_cbk",
           [](Local_MIP& self, py::function fn)
           {
             auto cbk = [fn](Start::Start_Ctx& ctx, void* p_user_data)
             {
               (void)p_user_data;
               PyCallbackGuard g;
               fn(py::capsule(&ctx));
             };
             self.set_start_cbk(cbk, nullptr);
           },
           py::arg("callable"))
      .def("set_restart_cbk",
           [](Local_MIP& self, py::function fn)
           {
             auto cbk = [fn](Restart::Restart_Ctx& ctx, void* p_user_data)
             {
               (void)p_user_data;
               PyCallbackGuard g;
               fn(py::capsule(&ctx));
             };
             self.set_restart_cbk(cbk, nullptr);
           },
           py::arg("callable"))
      .def("set_weight_cbk",
           [](Local_MIP& self, py::function fn)
           {
             auto cbk = [fn](Weight::Weight_Ctx& ctx, void* p_user_data)
             {
               (void)p_user_data;
               PyCallbackGuard g;
               fn(py::capsule(&ctx));
             };
             self.set_weight_cbk(cbk, nullptr);
           },
           py::arg("callable"))
      .def("set_lift_scoring_cbk",
           [](Local_MIP& self, py::function fn)
           {
             auto cbk = [fn](Scoring::Lift_Ctx& ctx,
                             size_t var_idx,
                             double delta,
                             void* p_user_data)
             {
               (void)p_user_data;
               PyCallbackGuard g;
               fn(py::capsule(&ctx), var_idx, delta);
             };
             self.set_lift_scoring_cbk(cbk, nullptr);
           },
           py::arg("callable"))
      .def("set_neighbor_scoring_cbk",
           [](Local_MIP& self, py::function fn)
           {
             auto cbk = [fn](Scoring::Neighbor_Ctx& ctx,
                             size_t var_idx,
                             double delta,
                             void* p_user_data)
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
      .def("is_feasible", &Local_MIP::is_feasible);
}
