/*=====================================================================================

    Filename:     MPS_Reader.cpp

    Description:
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../model_data/Model_Con.h"
#include "../model_data/Model_Var.h"
#include "../utils/global_defs.h"
#include "../utils/solver_error.h"
#include "MPS_Reader.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

MPS_Reader::MPS_Reader(Model_Manager* p_model_manager)
    : m_model_manager(p_model_manager), m_integrality_marker(false),
      m_small_coeff_counter(0)
{
}

bool MPS_Reader::read_optional_bound_value(double& p_value)
{
  std::string value_text;
  if (!(m_iss >> value_text))
    return false;

  std::istringstream value_stream(value_text);
  char trailing_char = '\0';
  if (!(value_stream >> p_value) || (value_stream >> trailing_char))
    printf_error_line(m_read_line);

  std::string extra_field;
  if (m_iss >> extra_field)
    printf_error_line(m_read_line);
  return true;
}

void MPS_Reader::read(const char* p_model_file)
{
  auto start_time = std::chrono::high_resolution_clock::now();
  m_integrality_marker = false;
  m_small_coeff_counter = 0;
  m_ignored_free_rows.clear();
  std::ifstream infile(p_model_file);
  if (!infile)
  {
    printf("c The model file %s is not found.\n", p_model_file);
    char message[256];
    std::snprintf(message,
                  sizeof(message),
                  "c The model file %s is not found.",
                  p_model_file);
    throw Solver_Error(message);
  }
  std::string model_name;
  std::string temp_str;
  char con_type;
  std::string con_name;
  std::string var_name;
  double coeff = 0.0;
  double rhs = 0.0;
  std::string bound_type;
  double input_bound = 0.0;

  auto read_next_record = [&]()
  {
    while (std::getline(infile, m_read_line))
    {
      if (m_read_line.empty() || m_read_line[0] == '*' ||
          is_blank(m_read_line))
        continue;
      return true;
    }
    return false;
  };

  if (!read_next_record())
    throw Solver_Error("c empty MPS file");

  iss_setup();
  if (!(m_iss >> temp_str) || temp_str != "NAME")
    printf_error_line(m_read_line);
  m_iss >> model_name;
  printf("c model name: %s\n", model_name.c_str());

  std::string requested_obj_name;
  if (!read_next_record())
    throw Solver_Error("c ROWS section is missing");
  while (true) // optional OBJSENSE and OBJNAME sections
  {
    iss_setup();
    if (!(m_iss >> temp_str))
      printf_error_line(m_read_line);
    if (temp_str == "ROWS")
      break;

    if (temp_str == "OBJSENSE")
    {
      std::string obj_sense;
      if (!(m_iss >> obj_sense))
      {
        if (!read_next_record())
          throw Solver_Error("c OBJSENSE value is missing");
        iss_setup();
        if (!(m_iss >> obj_sense))
          printf_error_line(m_read_line);
      }
      std::string extra_field;
      if (m_iss >> extra_field)
        printf_error_line(m_read_line);
      if (obj_sense == "MAX" || obj_sense == "MAXIMIZE")
        m_model_manager->setup_max();
      else if (obj_sense != "MIN" && obj_sense != "MINIMIZE")
        printf_error_line(m_read_line);
    }
    else if (temp_str == "OBJNAME")
    {
      if (!requested_obj_name.empty())
        printf_error_line(m_read_line);
      if (!(m_iss >> requested_obj_name))
      {
        if (!read_next_record())
          throw Solver_Error("c OBJNAME value is missing");
        iss_setup();
        if (!(m_iss >> requested_obj_name))
          printf_error_line(m_read_line);
      }
      std::string extra_field;
      if (m_iss >> extra_field)
        printf_error_line(m_read_line);
    }
    else
      printf_error_line(m_read_line);

    if (!read_next_record())
      throw Solver_Error("c ROWS section is missing");
  }

  std::vector<std::string> free_row_names;
  std::unordered_set<std::string> free_row_name_set;
  m_model_manager->make_con("");            // obj
  while (std::getline(infile, m_read_line)) // ROWS section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'C')
      break;
    iss_setup();
    if (!(m_iss >> con_type >> con_name))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }

    std::string extra_field;
    if (m_iss >> extra_field)
      printf_error_line(m_read_line);

    if (con_type == 'N')
    {
      if (!free_row_name_set.insert(con_name).second)
        printf_error_line(m_read_line);
      free_row_names.push_back(con_name);
    }
    else if (con_type == 'L')
      m_model_manager->make_con(con_name, '<');
    else if (con_type == 'E')
      m_model_manager->make_con(con_name, '=');
    else if (con_type == 'G')
      m_model_manager->make_con(con_name, '>');
    else
      printf_error_line(m_read_line);
  }

  std::string selected_obj_name;
  if (!requested_obj_name.empty())
  {
    if (!free_row_name_set.contains(requested_obj_name))
      throw Solver_Error("c OBJNAME row is not an N row: " +
                         requested_obj_name);
    selected_obj_name = requested_obj_name;
  }
  else if (!free_row_names.empty())
    selected_obj_name = free_row_names.front();

  if (!selected_obj_name.empty())
    m_model_manager->set_obj_name(selected_obj_name);
  for (const std::string& free_row_name : free_row_names)
    if (free_row_name != selected_obj_name)
      m_ignored_free_rows.insert(free_row_name);
  if (!m_ignored_free_rows.empty())
  {
    printf("c ignored %zu non-objective free rows.\n",
           m_ignored_free_rows.size());
  }

  while (std::getline(infile, m_read_line)) // COLUMNS section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'R')
      break;
    iss_setup();
    if (!(m_iss >> var_name >> con_name))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }
    if (con_name == "\'MARKER\'")
    {
      if (!(m_iss >> temp_str))
        printf_error_line(m_read_line);
      if (temp_str == "\'INTORG\'")
      {
        if (m_integrality_marker)
          printf_error_line(m_read_line);
        m_integrality_marker = true;
      }
      else if (temp_str == "\'INTEND\'")
      {
        if (!m_integrality_marker)
          printf_error_line(m_read_line);
        m_integrality_marker = false;
      }
      else
        printf_error_line(m_read_line);
      continue;
    }
    if (!(m_iss >> coeff))
      printf_error_line(m_read_line);
    if (!std::isfinite(coeff))
      printf_error_line(m_read_line);
    add_coeff_var_to_con(con_name, coeff, var_name);
    if (m_iss >> con_name)
    {
      if (!(m_iss >> coeff))
        printf_error_line(m_read_line);
      if (!std::isfinite(coeff))
        printf_error_line(m_read_line);
      add_coeff_var_to_con(con_name, coeff, var_name);
    }
  }
  if (m_integrality_marker)
    throw Solver_Error("c unterminated INTORG marker in COLUMNS section");
  std::string selected_rhs_name;
  auto apply_rhs_to_row =
      [&](const std::string& row_name, double rhs_value)
  {
    if (m_ignored_free_rows.contains(row_name))
      return;
    if (row_name == m_model_manager->get_obj_name())
      m_model_manager->con(0).set_rhs(rhs_value);
    else
      m_model_manager->set_rhs(row_name, rhs_value);
  };
  while (std::getline(infile, m_read_line)) // rhs  section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'B' || m_read_line[0] == 'E' ||
        m_read_line[0] == 'R')
      break;
    if (m_read_line[0] == 'S') // do not handle SOS
      printf_error_line(m_read_line);
    iss_setup();
    if (!(m_iss >> temp_str >> con_name >> rhs))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }
    if (!std::isfinite(rhs))
      printf_error_line(m_read_line);
    if (selected_rhs_name.empty())
      selected_rhs_name = temp_str;
    const bool use_rhs = temp_str == selected_rhs_name;
    if (use_rhs)
      apply_rhs_to_row(con_name, rhs);
    if (m_iss >> con_name)
    {
      if (!(m_iss >> rhs))
        printf_error_line(m_read_line);
      if (!std::isfinite(rhs))
        printf_error_line(m_read_line);
      if (use_rhs)
        apply_rhs_to_row(con_name, rhs);
    }
  }
  if (!m_read_line.empty() && m_read_line[0] == 'R') // RANGES section
  {
    size_t range_con_counter = 0;
    std::string selected_range_name;
    auto make_range_con_name =
        [&range_con_counter](const std::string& base_name)
    {
      return base_name + "_range_" + std::to_string(range_con_counter++);
    };
    auto add_range_constraint =
        [&](const Model_Con& source, double new_rhs, char new_symbol)
    {
      const size_t term_num = source.term_num();
      std::vector<std::pair<size_t, double>> terms;
      terms.reserve(term_num);
      for (size_t term_idx = 0; term_idx < term_num; ++term_idx)
        terms.emplace_back(source.var_idx(term_idx),
                           source.coeff(term_idx));
      const std::string new_name = make_range_con_name(source.name());
      size_t new_idx = m_model_manager->make_con(new_name, new_symbol);
      auto& new_con = m_model_manager->con(new_idx);
      new_con.set_rhs(new_rhs);
      for (const auto& [var_idx, term_coeff] : terms)
      {
        auto& var = m_model_manager->var(var_idx);
        var.add_con(new_idx, new_con.term_num());
        new_con.add_var(var_idx, term_coeff, var.term_num() - 1);
      }
    };
    auto apply_range_to_row =
        [&](const std::string& row_name, double range_value)
    {
      if (m_ignored_free_rows.contains(row_name))
        return;
      size_t con_idx = m_model_manager->con_idx(row_name);
      if (con_idx == 0)
        printf_error_line(m_read_line);
      auto& con = m_model_manager->con(con_idx);
      double rhs_value = con.rhs();
      double abs_range = std::fabs(range_value);
      if (con.is_equality())
      {
        double upper_rhs =
            range_value >= 0 ? rhs_value + range_value : rhs_value;
        double lower_rhs =
            range_value >= 0 ? rhs_value : rhs_value + range_value;
        con.convert_equality_to_less();
        con.set_rhs(upper_rhs);
        add_range_constraint(con, lower_rhs, '>');
      }
      else if (con.is_greater())
      {
        double upper_rhs = rhs_value + abs_range;
        add_range_constraint(con, upper_rhs, '<');
      }
      else
      {
        double lower_rhs = rhs_value - abs_range;
        add_range_constraint(con, lower_rhs, '>');
      }
    };
    while (std::getline(infile, m_read_line))
    {
      if (m_read_line.empty() || m_read_line[0] == '*')
        continue;
      if (m_read_line[0] == 'B' || m_read_line[0] == 'E')
        break;
      if (m_read_line[0] == 'S') // do not handle SOS
        printf_error_line(m_read_line);
      iss_setup();
      double range_value = 0.0;
      if (!(m_iss >> temp_str >> con_name >> range_value))
      {
        if (!is_blank(m_read_line))
          printf_error_line(m_read_line);
        else
          continue;
      }
      if (!std::isfinite(range_value))
        printf_error_line(m_read_line);
      if (selected_range_name.empty())
        selected_range_name = temp_str;
      const bool use_range = temp_str == selected_range_name;
      if (use_range)
        apply_range_to_row(con_name, range_value);
      while (m_iss >> con_name)
      {
        if (!(m_iss >> range_value))
          printf_error_line(m_read_line);
        if (!std::isfinite(range_value))
          printf_error_line(m_read_line);
        if (use_range)
          apply_range_to_row(con_name, range_value);
      }
    }
  }
  std::string selected_bound_name;
  std::unordered_set<std::string> lower_bound_vars;
  std::unordered_set<std::string> upper_bound_vars;
  while (std::getline(infile, m_read_line)) // BOUNDS section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'E')
      break;
    if (m_read_line[0] == 'I') // do not handle INDICATORS
      printf_error_line(m_read_line);
    iss_setup();
    if (!(m_iss >> bound_type >> temp_str >> var_name))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }
    const bool requires_value = bound_type == "UP" || bound_type == "LO" ||
                                bound_type == "LI" || bound_type == "UI" ||
                                bound_type == "FX";
    const bool supported_type = requires_value || bound_type == "BV" ||
                                bound_type == "FR" || bound_type == "MI" ||
                                bound_type == "PL";
    if (!supported_type)
      printf_error_line(m_read_line);

    const bool has_value = read_optional_bound_value(input_bound);
    if ((requires_value && !has_value) ||
        (has_value && !std::isfinite(input_bound)))
      printf_error_line(m_read_line);

    if (selected_bound_name.empty())
      selected_bound_name = temp_str;
    if (temp_str != selected_bound_name)
      continue;
    if (!m_model_manager->exists_var(var_name))
      printf_error_line(m_read_line);

    const bool sets_lower = bound_type == "LO" || bound_type == "LI" ||
                            bound_type == "FX" || bound_type == "FR" ||
                            bound_type == "MI" || bound_type == "BV";
    const bool sets_upper = bound_type == "UP" || bound_type == "UI" ||
                            bound_type == "FX" || bound_type == "FR" ||
                            bound_type == "PL" || bound_type == "BV";
    const bool has_explicit_lower = lower_bound_vars.contains(var_name);
    if ((sets_lower && has_explicit_lower) ||
        (sets_upper && upper_bound_vars.contains(var_name)))
      printf_error_line(m_read_line);
    if (sets_lower)
      lower_bound_vars.insert(var_name);
    if (sets_upper)
      upper_bound_vars.insert(var_name);

    auto& var = m_model_manager->var(var_name);
    if (bound_type != "BV" && var.type() == Var_Type::binary)
    {
      var.set_type(Var_Type::general_integer);
      var.set_upper_bound(k_inf);
    }
    if (bound_type == "UP")
    {
      if (input_bound < 0.0 && !has_explicit_lower)
        var.set_lower_bound(k_neg_inf);
      var.set_upper_bound(input_bound);
    }
    else if (bound_type == "LO")
      var.set_lower_bound(input_bound);
    else if (bound_type == "BV")
    {
      var.set_type(Var_Type::binary);
      var.set_upper_bound(1.0);
      var.set_lower_bound(0.0);
    }
    else if (bound_type == "LI")
    {
      if (!is_integral_within_tolerance(input_bound))
        printf_error_line(m_read_line);
      if (!var.requires_integrality())
        var.set_type(Var_Type::general_integer);
      var.set_lower_bound(input_bound);
    }
    else if (bound_type == "UI")
    {
      if (!is_integral_within_tolerance(input_bound))
        printf_error_line(m_read_line);
      if (!var.requires_integrality())
        var.set_type(Var_Type::general_integer);
      if (input_bound < 0.0 && !has_explicit_lower)
        var.set_lower_bound(k_neg_inf);
      var.set_upper_bound(input_bound);
    }
    else if (bound_type == "FX")
    {
      var.set_lower_bound(input_bound);
      var.set_upper_bound(input_bound);
      var.set_type(Var_Type::fixed);
    }
    else if (bound_type == "FR")
    {
      var.set_upper_bound(k_inf);
      var.set_lower_bound(k_neg_inf);
    }
    else if (bound_type == "MI")
      var.set_lower_bound(k_neg_inf);
    else if (bound_type == "PL")
      var.set_upper_bound(k_inf);
  }
  infile.close();
  if (m_small_coeff_counter > 0)
    printf("c skipped %zu coefficients smaller than %.3e.\n",
           m_small_coeff_counter,
           k_zero_tolerance);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  printf("c reading mps file takes %.2lf seconds.\n",
         duration.count() / 1000.0);
}

void MPS_Reader::add_coeff_var_to_con(const std::string& p_con_name,
                                      double p_coeff,
                                      const std::string& p_var_name)
{
  size_t var_idx =
      m_model_manager->make_var(p_var_name, m_integrality_marker);
  if (m_ignored_free_rows.contains(p_con_name))
    return;
  if (std::fabs(p_coeff) < k_zero_tolerance)
  {
    ++m_small_coeff_counter;
    return;
  }
  size_t con_idx;
  Model_Con* con;
  if (p_con_name == m_model_manager->get_obj_name())
  {
    con_idx = 0;
    con = &m_model_manager->con(0);
  }
  else
  {
    con_idx = m_model_manager->con_idx(p_con_name);
    con = &m_model_manager->con(con_idx);
  }
  auto& var = m_model_manager->var(var_idx);
  var.add_con(con_idx, con->term_num());
  con->add_var(var_idx, p_coeff, var.term_num() - 1);
}

void MPS_Reader::print_con(const Model_Con& p_con)
{
  printf("c %s: ", p_con.name().c_str());
  for (size_t i = 0; i < p_con.term_num(); ++i)
  {
    printf("%lf * %s",
           p_con.coeff(i),
           m_model_manager->var(p_con.var_idx(i)).name().c_str());
    if (i < p_con.term_num() - 1)
      printf(" + ");
  }
  printf(" %c %lf\n", p_con.is_equality() ? '=' : '<', p_con.rhs());
}
