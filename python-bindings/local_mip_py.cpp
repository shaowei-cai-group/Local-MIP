#include "../src/local_mip/Local_MIP.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace py = pybind11;

namespace
{

struct PyCallbackGuard
{
  py::gil_scoped_acquire lock;
  PyCallbackGuard() = default;
};

template <typename T>
py::list vector_to_list(const std::vector<T>& values)
{
  py::list result;
  for (const auto& value : values)
    result.append(py::cast(value));
  return result;
}

template <>
py::list vector_to_list<bool>(const std::vector<bool>& values)
{
  py::list result;
  for (bool value : values)
    result.append(py::bool_(value));
  return result;
}

template <typename T>
class ReadonlyVectorView
{
public:
  explicit ReadonlyVectorView(const std::vector<T>* p_values)
      : m_values(p_values)
  {
  }

  size_t size() const
  {
    return m_values->size();
  }

  T get(size_t p_idx) const
  {
    check_index(p_idx);
    return (*m_values)[p_idx];
  }

  py::list to_list() const
  {
    return vector_to_list(*m_values);
  }

  const std::vector<T>& values() const
  {
    return *m_values;
  }

private:
  void check_index(size_t p_idx) const
  {
    if (p_idx >= m_values->size())
      throw py::index_error("index out of range");
  }

  const std::vector<T>* m_values;
};

using ReadonlyBoolVectorView = ReadonlyVectorView<bool>;
using ReadonlyDoubleVectorView = ReadonlyVectorView<double>;
using ReadonlySizeVectorView = ReadonlyVectorView<size_t>;

class DoubleVectorView
{
public:
  explicit DoubleVectorView(std::vector<double>* p_values)
      : m_values(p_values)
  {
  }

  size_t size() const
  {
    return m_values->size();
  }

  double get(size_t p_idx) const
  {
    check_index(p_idx);
    return (*m_values)[p_idx];
  }

  void set(size_t p_idx, double p_value)
  {
    check_index(p_idx);
    (*m_values)[p_idx] = p_value;
  }

  py::list to_list() const
  {
    return vector_to_list(*m_values);
  }

  const std::vector<double>& values() const
  {
    return *m_values;
  }

private:
  void check_index(size_t p_idx) const
  {
    if (p_idx >= m_values->size())
      throw py::index_error("index out of range");
  }

  std::vector<double>* m_values;
};

class SizeVectorView
{
public:
  explicit SizeVectorView(std::vector<size_t>* p_values)
      : m_values(p_values)
  {
  }

  size_t size() const
  {
    return m_values->size();
  }

  size_t get(size_t p_idx) const
  {
    check_index(p_idx);
    return (*m_values)[p_idx];
  }

  void set(size_t p_idx, size_t p_value)
  {
    check_index(p_idx);
    (*m_values)[p_idx] = p_value;
  }

  py::list to_list() const
  {
    return vector_to_list(*m_values);
  }

  const std::vector<size_t>& values() const
  {
    return *m_values;
  }

private:
  void check_index(size_t p_idx) const
  {
    if (p_idx >= m_values->size())
      throw py::index_error("index out of range");
  }

  std::vector<size_t>* m_values;
};

class UInt32VectorView
{
public:
  explicit UInt32VectorView(std::vector<uint32_t>* p_values)
      : m_values(p_values)
  {
  }

  size_t size() const
  {
    return m_values->size();
  }

  uint32_t get(size_t p_idx) const
  {
    check_index(p_idx);
    return (*m_values)[p_idx];
  }

  void set(size_t p_idx, uint32_t p_value)
  {
    check_index(p_idx);
    (*m_values)[p_idx] = p_value;
  }

  py::list to_list() const
  {
    return vector_to_list(*m_values);
  }

  const std::vector<uint32_t>& values() const
  {
    return *m_values;
  }

private:
  void check_index(size_t p_idx) const
  {
    if (p_idx >= m_values->size())
      throw py::index_error("index out of range");
  }

  std::vector<uint32_t>* m_values;
};

class RandomView
{
public:
  explicit RandomView(std::mt19937* p_rng) : m_rng(p_rng) {}

  double random() const
  {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(*m_rng);
  }

  long long randint(long long p_low, long long p_high) const
  {
    if (p_low > p_high)
      throw py::value_error("low must be <= high");
    std::uniform_int_distribution<long long> dist(p_low, p_high);
    return dist(*m_rng);
  }

  double uniform(double p_low, double p_high) const
  {
    if (p_low > p_high)
      throw py::value_error("low must be <= high");
    std::uniform_real_distribution<double> dist(p_low, p_high);
    return dist(*m_rng);
  }

private:
  std::mt19937* m_rng;
};

struct PyCallbackState
{
  py::function fn;
  py::object user_data;
  bool has_user_data;
};

std::shared_ptr<PyCallbackState>
make_callback_state(py::function p_fn)
{
  return std::make_shared<PyCallbackState>(
      PyCallbackState{std::move(p_fn), py::none(), false});
}

std::shared_ptr<PyCallbackState>
make_callback_state(py::function p_fn, py::object p_user_data)
{
  return std::make_shared<PyCallbackState>(
      PyCallbackState{std::move(p_fn),
                      std::move(p_user_data),
                      true});
}

py::weakref make_callback_owner_ref(Local_MIP& self)
{
  return py::weakref(py::cast(&self, py::return_value_policy::reference));
}

template <typename T>
py::object cast_callback_ctx(T* p_ctx, const py::weakref& owner_ref)
{
  py::object owner = owner_ref();
  if (owner.is_none())
    throw std::runtime_error("callback owner is no longer available");
  return py::cast(p_ctx, py::return_value_policy::reference_internal, owner);
}

} // namespace

PYBIND11_MODULE(localmip_py, m)
{
  m.doc() = "Python bindings for Local-MIP";

  py::class_<ReadonlyBoolVectorView>(m, "ReadonlyBoolVectorView")
      .def("__len__", &ReadonlyBoolVectorView::size)
      .def("__getitem__", &ReadonlyBoolVectorView::get)
      .def("to_list", &ReadonlyBoolVectorView::to_list)
      .def("__iter__",
           [](const ReadonlyBoolVectorView& self)
           { return vector_to_list(self.values()).attr("__iter__")(); });

  py::class_<ReadonlyDoubleVectorView>(m, "ReadonlyDoubleVectorView")
      .def("__len__", &ReadonlyDoubleVectorView::size)
      .def("__getitem__", &ReadonlyDoubleVectorView::get)
      .def("to_list", &ReadonlyDoubleVectorView::to_list)
      .def("__iter__",
           [](const ReadonlyDoubleVectorView& self)
           { return py::make_iterator(self.values().begin(),
                                      self.values().end()); },
           py::keep_alive<0, 1>());

  py::class_<ReadonlySizeVectorView>(m, "ReadonlySizeVectorView")
      .def("__len__", &ReadonlySizeVectorView::size)
      .def("__getitem__", &ReadonlySizeVectorView::get)
      .def("to_list", &ReadonlySizeVectorView::to_list)
      .def("__iter__",
           [](const ReadonlySizeVectorView& self)
           { return py::make_iterator(self.values().begin(),
                                      self.values().end()); },
           py::keep_alive<0, 1>());

  py::class_<DoubleVectorView>(m, "DoubleVectorView")
      .def("__len__", &DoubleVectorView::size)
      .def("__getitem__", &DoubleVectorView::get)
      .def("__setitem__", &DoubleVectorView::set)
      .def("to_list", &DoubleVectorView::to_list)
      .def("__iter__",
           [](const DoubleVectorView& self)
           { return py::make_iterator(self.values().begin(),
                                      self.values().end()); },
           py::keep_alive<0, 1>());

  py::class_<SizeVectorView>(m, "SizeVectorView")
      .def("__len__", &SizeVectorView::size)
      .def("__getitem__", &SizeVectorView::get)
      .def("__setitem__", &SizeVectorView::set)
      .def("to_list", &SizeVectorView::to_list)
      .def("__iter__",
           [](const SizeVectorView& self)
           { return py::make_iterator(self.values().begin(),
                                      self.values().end()); },
           py::keep_alive<0, 1>());

  py::class_<UInt32VectorView>(m, "UInt32VectorView")
      .def("__len__", &UInt32VectorView::size)
      .def("__getitem__", &UInt32VectorView::get)
      .def("__setitem__", &UInt32VectorView::set)
      .def("to_list", &UInt32VectorView::to_list)
      .def("__iter__",
           [](const UInt32VectorView& self)
           { return py::make_iterator(self.values().begin(),
                                      self.values().end()); },
           py::keep_alive<0, 1>());

  py::class_<RandomView>(m, "RandomStream")
      .def("random", &RandomView::random)
      .def("randint", &RandomView::randint)
      .def("uniform", &RandomView::uniform);

  py::enum_<Model_API::Sense>(m, "Sense")
      .value("minimize", Model_API::Sense::minimize)
      .value("maximize", Model_API::Sense::maximize);

  py::enum_<Var_Type>(m, "VarType")
      .value("binary", Var_Type::binary)
      .value("general_integer", Var_Type::general_integer)
      .value("real", Var_Type::real)
      .value("fixed", Var_Type::fixed);

  py::class_<Model_Var>(m, "ModelVar")
      .def_property_readonly("name", &Model_Var::name)
      .def_property_readonly("idx", &Model_Var::idx)
      .def_property_readonly("type", &Model_Var::type)
      .def_property_readonly("term_num", &Model_Var::term_num)
      .def_property_readonly("upper_bound", &Model_Var::upper_bound)
      .def_property_readonly("lower_bound", &Model_Var::lower_bound)
      .def("in_bound", &Model_Var::in_bound, py::arg("value"))
      .def("is_fixed", &Model_Var::is_fixed)
      .def("is_binary", &Model_Var::is_binary)
      .def("is_real", &Model_Var::is_real)
      .def("is_general_integer", &Model_Var::is_general_integer)
      .def("pos_in_con", &Model_Var::pos_in_con, py::arg("term_idx"))
      .def("con_idx", &Model_Var::con_idx, py::arg("term_idx"))
      .def("con_idx_set",
           [](const Model_Var& self)
           { return vector_to_list(self.con_idx_set()); });

  py::class_<Model_Con>(m, "ModelCon")
      .def_property_readonly("name", &Model_Con::name)
      .def_property_readonly("idx", &Model_Con::idx)
      .def_property_readonly("term_num", &Model_Con::term_num)
      .def_property_readonly("is_equality", &Model_Con::is_equality)
      .def_property_readonly("is_greater", &Model_Con::is_greater)
      .def_property_readonly("rhs", &Model_Con::rhs)
      .def_property_readonly("is_inferred_sat", &Model_Con::is_inferred_sat)
      .def("coeff", &Model_Con::coeff, py::arg("term_idx"))
      .def("var_idx", &Model_Con::var_idx, py::arg("term_idx"))
      .def("var_idx_set",
           [](const Model_Con& self)
           { return vector_to_list(self.var_idx_set()); })
      .def("coeff_set",
           [](const Model_Con& self)
           { return vector_to_list(self.coeff_set()); });

  py::class_<Model_Manager>(m, "ModelManager")
      .def_property_readonly("obj_name", &Model_Manager::get_obj_name)
      .def_property_readonly("var_num", &Model_Manager::var_num)
      .def_property_readonly("con_num", &Model_Manager::con_num)
      .def_property_readonly("general_integer_num",
                             &Model_Manager::general_integer_num)
      .def_property_readonly("binary_num", &Model_Manager::binary_num)
      .def_property_readonly("fixed_num", &Model_Manager::fixed_num)
      .def_property_readonly("real_num", &Model_Manager::real_num)
      .def_property_readonly("is_min",
                             [](const Model_Manager& self)
                             { return self.is_min() > 0; })
      .def_property_readonly("obj_offset", &Model_Manager::obj_offset)
      .def("var",
           [](const Model_Manager& self, size_t p_idx) -> const Model_Var&
           {
             if (p_idx >= self.var_num())
               throw py::index_error("variable index out of range");
             return self.var(p_idx);
           },
           py::arg("idx"),
           py::return_value_policy::reference_internal)
      .def("con",
           [](const Model_Manager& self, size_t p_idx) -> const Model_Con&
           {
             if (p_idx >= self.con_num())
               throw py::index_error("constraint index out of range");
             return self.con(p_idx);
           },
           py::arg("idx"),
           py::return_value_policy::reference_internal)
      .def("obj",
           [](const Model_Manager& self) -> const Model_Con&
           {
             if (self.con_num() == 0)
               throw py::value_error("objective is not available yet");
             return self.obj();
           },
           py::return_value_policy::reference_internal)
      .def("exists_var", &Model_Manager::exists_var, py::arg("name"))
      .def("con_idx",
           [](const Model_Manager& self, const std::string& p_name)
           {
             for (size_t idx = 0; idx < self.con_num(); ++idx)
             {
               if (self.con(idx).name() == p_name)
                 return idx;
             }
             throw py::key_error("unknown constraint name: " + p_name);
           },
           py::arg("name"))
      .def("var_id_to_obj_idx",
           [](const Model_Manager& self, size_t p_var_idx)
           {
             if (p_var_idx >= self.var_num())
               throw py::index_error("variable index out of range");
             return self.var_id_to_obj_idx(p_var_idx);
           },
           py::arg("var_idx"))
      .def_property_readonly("binary_indices",
                             [](const Model_Manager& self)
                             {
                               return ReadonlySizeVectorView(
                                   &self.binary_idx_list());
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly(
          "con_is_equality",
          [](const Model_Manager& self)
          {
            return ReadonlyBoolVectorView(&self.con_is_equality());
          },
          py::keep_alive<0, 1>())
      .def_property_readonly(
          "non_fixed_var_indices",
          [](const Model_Manager& self)
          {
            return ReadonlySizeVectorView(&self.non_fixed_var_idxs());
          },
          py::keep_alive<0, 1>())
      .def_property_readonly("var_obj_cost",
                             [](const Model_Manager& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.var_obj_cost());
                             },
                             py::keep_alive<0, 1>());

  py::class_<Readonly_Ctx>(m, "ReadonlyCtx")
      .def_property_readonly(
          "model_manager",
          [](const Readonly_Ctx& self) -> const Model_Manager&
          { return self.m_model_manager; },
          py::return_value_policy::reference_internal)
      .def_property_readonly("var_current_value",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.m_var_current_value);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("var_best_value",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.m_var_best_value);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_activity",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.m_con_activity);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_constant",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.m_con_constant);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_is_equality",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyBoolVectorView(
                                   &self.m_con_is_equality);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_weight",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlySizeVectorView(
                                   &self.m_con_weight);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_unsat_idxs",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlySizeVectorView(
                                   &self.m_con_unsat_idxs);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly(
          "con_pos_in_unsat_idxs",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_con_pos_in_unsat_idxs);
          },
          py::keep_alive<0, 1>())
      .def_property_readonly("con_sat_idxs",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlySizeVectorView(
                                   &self.m_con_sat_idxs);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly(
          "var_last_dec_step",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_var_last_dec_step);
          },
          py::keep_alive<0, 1>())
      .def_property_readonly(
          "var_last_inc_step",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_var_last_inc_step);
          },
          py::keep_alive<0, 1>())
      .def_property_readonly(
          "var_allow_inc_step",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_var_allow_inc_step);
          },
          py::keep_alive<0, 1>())
      .def_property_readonly(
          "var_allow_dec_step",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_var_allow_dec_step);
          },
          py::keep_alive<0, 1>())
      .def_property_readonly("obj_var_num",
                             [](const Readonly_Ctx& self)
                             { return self.m_obj_var_num; })
      .def_property_readonly("var_obj_cost",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlyDoubleVectorView(
                                   &self.m_var_obj_cost);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly("is_found_feasible",
                             [](const Readonly_Ctx& self)
                             { return self.m_is_found_feasible; })
      .def_property_readonly("best_obj",
                             [](const Readonly_Ctx& self)
                             { return self.m_best_obj; })
      .def_property_readonly("current_obj_breakthrough",
                             [](const Readonly_Ctx& self)
                             { return self.m_current_obj_breakthrough; })
      .def_property_readonly("cur_step",
                             [](const Readonly_Ctx& self)
                             { return self.m_cur_step; })
      .def_property_readonly("last_improve_step",
                             [](const Readonly_Ctx& self)
                             { return self.m_last_improve_step; })
      .def_property_readonly("binary_idx_list",
                             [](const Readonly_Ctx& self)
                             {
                               return ReadonlySizeVectorView(
                                   &self.m_binary_idx_list);
                             },
                             py::keep_alive<0, 1>())
      .def_property_readonly(
          "non_fixed_var_idx_list",
          [](const Readonly_Ctx& self)
          {
            return ReadonlySizeVectorView(&self.m_non_fixed_var_idx_list);
          },
          py::keep_alive<0, 1>());

  py::class_<Start::Start_Ctx>(m, "StartCtx")
      .def_property_readonly(
          "shared",
          [](Start::Start_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "current_values",
          [](Start::Start_Ctx& self)
          { return DoubleVectorView(&self.m_var_current_value); },
          py::keep_alive<0, 1>())
      .def_property_readonly("rng",
                             [](Start::Start_Ctx& self)
                             { return RandomView(&self.m_rng); },
                             py::keep_alive<0, 1>());

  py::class_<Restart::Restart_Ctx>(m, "RestartCtx")
      .def_property_readonly(
          "shared",
          [](Restart::Restart_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "current_values",
          [](Restart::Restart_Ctx& self)
          { return DoubleVectorView(&self.m_var_current_value); },
          py::keep_alive<0, 1>())
      .def_property_readonly("rng",
                             [](Restart::Restart_Ctx& self)
                             { return RandomView(&self.m_rng); },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_weight",
                             [](Restart::Restart_Ctx& self)
                             { return SizeVectorView(&self.m_con_weight); },
                             py::keep_alive<0, 1>());

  py::class_<Weight::Weight_Ctx>(m, "WeightCtx")
      .def_property_readonly(
          "shared",
          [](Weight::Weight_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly("rng",
                             [](Weight::Weight_Ctx& self)
                             { return RandomView(&self.m_rng); },
                             py::keep_alive<0, 1>())
      .def_property_readonly("con_weight",
                             [](Weight::Weight_Ctx& self)
                             { return SizeVectorView(&self.m_con_weight); },
                             py::keep_alive<0, 1>());

  py::class_<Scoring::Lift_Ctx>(m, "LiftCtx")
      .def_property_readonly(
          "shared",
          [](Scoring::Lift_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly("rng",
                             [](Scoring::Lift_Ctx& self)
                             { return RandomView(&self.m_rng); },
                             py::keep_alive<0, 1>())
      .def_property("best_lift_score",
                    [](Scoring::Lift_Ctx& self)
                    { return self.m_best_lift_score; },
                    [](Scoring::Lift_Ctx& self, double p_value)
                    { self.m_best_lift_score = p_value; })
      .def_property("best_var_idx",
                    [](Scoring::Lift_Ctx& self)
                    { return self.m_best_var_idx; },
                    [](Scoring::Lift_Ctx& self, size_t p_value)
                    { self.m_best_var_idx = p_value; })
      .def_property("best_delta",
                    [](Scoring::Lift_Ctx& self)
                    { return self.m_best_delta; },
                    [](Scoring::Lift_Ctx& self, double p_value)
                    { self.m_best_delta = p_value; })
      .def_property("best_age",
                    [](Scoring::Lift_Ctx& self)
                    { return self.m_best_age; },
                    [](Scoring::Lift_Ctx& self, size_t p_value)
                    { self.m_best_age = p_value; });

  py::class_<Scoring::Neighbor_Ctx>(m, "NeighborScoringCtx")
      .def_property_readonly(
          "shared",
          [](Scoring::Neighbor_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "binary_op_stamp",
          [](Scoring::Neighbor_Ctx& self)
          { return UInt32VectorView(&self.m_binary_op_stamp); },
          py::keep_alive<0, 1>())
      .def_property("binary_op_stamp_token",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_binary_op_stamp_token; },
                    [](Scoring::Neighbor_Ctx& self, uint32_t p_value)
                    { self.m_binary_op_stamp_token = p_value; })
      .def_property("best_neighbor_score",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_best_neighbor_score; },
                    [](Scoring::Neighbor_Ctx& self, long p_value)
                    { self.m_best_neighbor_score = p_value; })
      .def_property("best_neighbor_subscore",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_best_neighbor_subscore; },
                    [](Scoring::Neighbor_Ctx& self, long p_value)
                    { self.m_best_neighbor_subscore = p_value; })
      .def_property("best_age",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_best_age; },
                    [](Scoring::Neighbor_Ctx& self, size_t p_value)
                    { self.m_best_age = p_value; })
      .def_property("best_var_idx",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_best_var_idx; },
                    [](Scoring::Neighbor_Ctx& self, size_t p_value)
                    { self.m_best_var_idx = p_value; })
      .def_property("best_delta",
                    [](Scoring::Neighbor_Ctx& self)
                    { return self.m_best_delta; },
                    [](Scoring::Neighbor_Ctx& self, double p_value)
                    { self.m_best_delta = p_value; });

  py::class_<Neighbor::Neighbor_Ctx>(m, "NeighborCtx")
      .def_property_readonly(
          "shared",
          [](Neighbor::Neighbor_Ctx& self) -> const Readonly_Ctx&
          { return self.m_shared; },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "op_var_idxs",
          [](Neighbor::Neighbor_Ctx& self)
          { return SizeVectorView(&self.m_op_var_idxs); },
          py::keep_alive<0, 1>())
      .def_property_readonly(
          "op_var_deltas",
          [](Neighbor::Neighbor_Ctx& self)
          { return DoubleVectorView(&self.m_op_var_deltas); },
          py::keep_alive<0, 1>())
      .def_property_readonly("op_size",
                             [](Neighbor::Neighbor_Ctx& self)
                             { return self.m_op_size; })
      .def_property_readonly("rng",
                             [](Neighbor::Neighbor_Ctx& self)
                             { return RandomView(&self.m_rng); },
                             py::keep_alive<0, 1>())
      .def("clear_ops", &Neighbor::Neighbor_Ctx::clear_ops)
      .def("set_single_op",
           &Neighbor::Neighbor_Ctx::set_single_op,
           py::arg("var_idx"),
           py::arg("delta"))
      .def("append_op",
           &Neighbor::Neighbor_Ctx::append_op,
           py::arg("var_idx"),
           py::arg("delta"));

  py::class_<Local_MIP>(m, "LocalMIP")
      .def(py::init<>())

      // Basic configuration
      .def("set_model_file", &Local_MIP::set_model_file, py::arg("path"))
      .def("set_param_set_file",
           &Local_MIP::set_param_set_file,
           py::arg("path"))
      .def("set_time_limit",
           &Local_MIP::set_time_limit,
           py::arg("seconds"))
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
      .def("set_bms_unsat_con",
           &Local_MIP::set_bms_unsat_con,
           py::arg("value"))
      .def("set_bms_mtm_unsat_op",
           &Local_MIP::set_bms_mtm_unsat_op,
           py::arg("value"))
      .def("set_bms_sat_con",
           &Local_MIP::set_bms_sat_con,
           py::arg("value"))
      .def("set_bms_mtm_sat_op",
           &Local_MIP::set_bms_mtm_sat_op,
           py::arg("value"))
      .def("set_bms_flip_op",
           &Local_MIP::set_bms_flip_op,
           py::arg("value"))
      .def("set_bms_easy_op",
           &Local_MIP::set_bms_easy_op,
           py::arg("value"))
      .def("set_bms_random_op",
           &Local_MIP::set_bms_random_op,
           py::arg("value"))

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
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Neighbor::Neighbor_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.add_custom_neighbor(name, cbk, nullptr);
          },
          py::arg("name"),
          py::arg("callable"))
      .def(
          "add_custom_neighbor",
          [](Local_MIP& self,
             const std::string& name,
             py::function fn,
             py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Neighbor::Neighbor_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.add_custom_neighbor(name, cbk, nullptr);
          },
          py::arg("name"),
          py::arg("callable"),
          py::arg("user_data"))
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
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Start::Start_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_start_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_start_cbk",
          [](Local_MIP& self, py::function fn, py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Start::Start_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_start_cbk(cbk, nullptr);
          },
          py::arg("callable"),
          py::arg("user_data"))
      .def(
          "set_restart_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Restart::Restart_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_restart_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_restart_cbk",
          [](Local_MIP& self, py::function fn, py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Restart::Restart_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_restart_cbk(cbk, nullptr);
          },
          py::arg("callable"),
          py::arg("user_data"))
      .def(
          "set_weight_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Weight::Weight_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_weight_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_weight_cbk",
          [](Local_MIP& self, py::function fn, py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Weight::Weight_Ctx& ctx,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, state->user_data);
              else
                state->fn(ctx_obj);
            };
            self.set_weight_cbk(cbk, nullptr);
          },
          py::arg("callable"),
          py::arg("user_data"))
      .def(
          "set_lift_scoring_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Scoring::Lift_Ctx& ctx,
                                          size_t var_idx,
                                          double delta,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, var_idx, delta, state->user_data);
              else
                state->fn(ctx_obj, var_idx, delta);
            };
            self.set_lift_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_lift_scoring_cbk",
          [](Local_MIP& self, py::function fn, py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Scoring::Lift_Ctx& ctx,
                                          size_t var_idx,
                                          double delta,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, var_idx, delta, state->user_data);
              else
                state->fn(ctx_obj, var_idx, delta);
            };
            self.set_lift_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"),
          py::arg("user_data"))
      .def(
          "set_neighbor_scoring_cbk",
          [](Local_MIP& self, py::function fn)
          {
            auto state = make_callback_state(std::move(fn));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Scoring::Neighbor_Ctx& ctx,
                                          size_t var_idx,
                                          double delta,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, var_idx, delta, state->user_data);
              else
                state->fn(ctx_obj, var_idx, delta);
            };
            self.set_neighbor_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"))
      .def(
          "set_neighbor_scoring_cbk",
          [](Local_MIP& self, py::function fn, py::object user_data)
          {
            auto state =
                make_callback_state(std::move(fn), std::move(user_data));
            auto owner_ref = make_callback_owner_ref(self);
            auto cbk = [state, owner_ref](Scoring::Neighbor_Ctx& ctx,
                                          size_t var_idx,
                                          double delta,
                                          void* /*user_data*/) mutable
            {
              PyCallbackGuard guard;
              py::object ctx_obj = cast_callback_ctx(&ctx, owner_ref);
              if (state->has_user_data)
                state->fn(ctx_obj, var_idx, delta, state->user_data);
              else
                state->fn(ctx_obj, var_idx, delta);
            };
            self.set_neighbor_scoring_cbk(cbk, nullptr);
          },
          py::arg("callable"),
          py::arg("user_data"))

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
      .def("get_solution", &Local_MIP::get_solution)
      .def("get_model_manager",
           &Local_MIP::get_model_manager,
           py::return_value_policy::reference_internal);
}
