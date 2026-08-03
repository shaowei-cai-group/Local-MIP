/*=====================================================================================

    Filename:     Prepared_Model.h

    Description:  Immutable, shareable model prepared for Local-MIP search
        Version:  2.0

=====================================================================================*/

#pragma once

#include "Model_Manager.h"
#include <memory>
#include <string>

class Model_API;

struct Model_Prepare_Options
{
  double feas_tolerance = k_default_feas_tolerance;

  double zero_tolerance = k_default_zero_tolerance;

  int bound_strengthen = 1;

  bool split_eq = true;
};

class Prepared_Model
{
private:
  std::unique_ptr<const Model_Manager> m_model_manager;

  explicit Prepared_Model(
      std::unique_ptr<const Model_Manager> p_model_manager);

  static void validate_options(const Model_Prepare_Options& p_options);

  friend class Model_API;

public:
  static std::shared_ptr<const Prepared_Model>
  from_file(const std::string& p_model_file,
            const Model_Prepare_Options& p_options = {});

  const Model_Manager& model_manager() const noexcept;
};
