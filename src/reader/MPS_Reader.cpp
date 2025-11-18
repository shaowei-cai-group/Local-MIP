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

MPS_Reader::MPS_Reader(Model_Manager* p_model_manager)
    : m_model_manager(p_model_manager), m_integrality_marker(false)
{
}

void MPS_Reader::read(const char* p_model_file)
{
  auto start_time = std::chrono::high_resolution_clock::now();
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
  double coeff;
  double rhs;
  std::string bound_type;
  double input_bound;
  while (std::getline(infile, m_read_line)) // NAME section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'R' || m_read_line[0] == 'O')
      break;
    iss_setup();
    if (!(m_iss >> temp_str))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }
    if (temp_str != "NAME")
      printf_error_line(m_read_line);
    model_name = "";
    m_iss >> model_name;
    printf("c model name: %s\n", model_name.c_str());
  }
  if (m_read_line[0] == 'O')
  {
    if (m_read_line.find("MAX") != std::string::npos)
      m_model_manager->setup_max();
    while (std::getline(infile, m_read_line))
    {
      if (m_read_line.empty() || m_read_line[0] == '*')
        continue;
      if (m_read_line[0] == 'R')
        break;
      iss_setup();
      m_iss >> temp_str;
      if (temp_str == "MAX")
        m_model_manager->setup_max();
    }
  }
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
    if (con_type == 'L')
      m_model_manager->make_con(con_name, '<');
    else if (con_type == 'E')
      m_model_manager->make_con(con_name, '=');
    else if (con_type == 'G')
      m_model_manager->make_con(con_name, '>');
    else
    {
      assert(con_type == 'N'); // type=='N',this con is obj
      if (m_model_manager->get_obj_name() != "")
        printf_error_line(m_read_line);
      m_model_manager->set_obj_name(con_name);
    }
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
      m_iss >> temp_str;
      if (temp_str != "\'INTORG\'" && temp_str != "\'INTEND\'")
        printf_error_line(m_read_line);
      m_integrality_marker = !m_integrality_marker;
      continue;
    }
    if (!(m_iss >> coeff))
      printf_error_line(m_read_line);
    add_coeff_var_to_con(con_name, coeff, var_name);
    if (m_iss >> con_name)
    {
      if (!(m_iss >> coeff))
        printf_error_line(m_read_line);
      add_coeff_var_to_con(con_name, coeff, var_name);
    }
  }
  while (std::getline(infile, m_read_line)) // rhs  section
  {
    if (m_read_line.empty() || m_read_line[0] == '*')
      continue;
    if (m_read_line[0] == 'B' || m_read_line[0] == 'E')
      break;
    if (m_read_line[0] == 'R' ||
        m_read_line[0] == 'S') // do not handle RANGS and SOS
      printf_error_line(m_read_line);
    iss_setup();
    if (!(m_iss >> temp_str >> con_name >> rhs))
    {
      if (!is_blank(m_read_line))
        printf_error_line(m_read_line);
      else
        continue;
    }
    m_model_manager->set_rhs(con_name, rhs);
    if (m_iss >> con_name)
    {
      if (!(m_iss >> rhs))
        printf_error_line(m_read_line);
      m_model_manager->set_rhs(con_name, rhs);
    }
  }
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
    m_iss >> input_bound;
    if (m_model_manager->exists_var(var_name))
    {
      auto& var = m_model_manager->var(var_name);
      if (var.type() == Var_Type::binary)
      {
        var.set_type(Var_Type::general_integer);
        var.set_upper_bound(k_inf);
      }
      if (bound_type == "UP")
        var.set_upper_bound(input_bound);
      else if (bound_type == "LO")
        var.set_lower_bound(input_bound);
      else if (bound_type == "BV")
      {
        var.set_type(Var_Type::binary);
        var.set_upper_bound(1.0);
        var.set_lower_bound(0.0);
      }
      else if (bound_type == "LI")
        var.set_lower_bound(input_bound);
      else if (bound_type == "UI")
        var.set_upper_bound(input_bound);
      else if (bound_type == "FX")
      {
        if (!var.is_real() &&
            std::fabs(input_bound - std::round(input_bound)) >
                k_feas_tolerance)
          var.set_type(Var_Type::real);
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
    else
      continue;
  }
  infile.close();
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
  if (std::fabs(p_coeff) < k_zero_tolerance)
  {
    printf("c coefficient is too small %lf, skipping...\n", p_coeff);
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
  size_t var_idx =
      m_model_manager->make_var(p_var_name, m_integrality_marker);
  auto& var = m_model_manager->var(var_idx);
  var.add_con(con_idx, con->term_num());
  if (con_idx == 0)
    p_coeff *= m_model_manager->is_min();
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
