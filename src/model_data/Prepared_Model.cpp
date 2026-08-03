/*=====================================================================================

    Filename:     Prepared_Model.cpp

    Description:  Immutable, shareable model prepared for Local-MIP search
        Version:  2.0

=====================================================================================*/

#include "../reader/Model_Reader.h"
#include "../utils/solver_error.h"
#include "Prepared_Model.h"
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

void Prepared_Model::validate_options(
    const Model_Prepare_Options& p_options)
{
  if (!std::isfinite(p_options.feas_tolerance) ||
      p_options.feas_tolerance < 0.0 ||
      p_options.feas_tolerance > k_max_feas_tolerance)
  {
    throw std::invalid_argument(
        "feas_tolerance must be finite and in [0, " +
        std::to_string(k_max_feas_tolerance) + "]");
  }
  if (!std::isfinite(p_options.zero_tolerance) ||
      p_options.zero_tolerance < 0.0 ||
      p_options.zero_tolerance > k_max_zero_tolerance)
  {
    throw std::invalid_argument(
        "zero_tolerance must be finite and in [0, " +
        std::to_string(k_max_zero_tolerance) + "]");
  }
  if (p_options.bound_strengthen < 0 || p_options.bound_strengthen > 2)
  {
    throw std::invalid_argument("bound_strengthen must be 0, 1, or 2");
  }
}

Prepared_Model::Prepared_Model(
    std::unique_ptr<const Model_Manager> p_model_manager)
    : m_model_manager(std::move(p_model_manager))
{
  if (m_model_manager == nullptr)
    throw std::invalid_argument("prepared model manager cannot be null");
}

std::shared_ptr<const Prepared_Model>
Prepared_Model::from_file(const std::string& p_model_file,
                          const Model_Prepare_Options& p_options)
{
  Prepared_Model::validate_options(p_options);
  auto manager = std::make_unique<Model_Manager>(p_options.feas_tolerance,
                                                 p_options.zero_tolerance);
  manager->set_bound_strengthen(p_options.bound_strengthen);
  manager->set_split_eq(p_options.split_eq);
  read_model_file(p_model_file, *manager);
  if (!manager->process_after_read())
    throw Solver_Error("model is infeasible during preparation");
  if (manager->var_num() == 0)
    throw Solver_Error("model must contain at least one variable");

  return std::shared_ptr<const Prepared_Model>(
      new Prepared_Model(std::move(manager)));
}

const Model_Manager& Prepared_Model::model_manager() const noexcept
{
  return *m_model_manager;
}
