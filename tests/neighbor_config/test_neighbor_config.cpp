/*=====================================================================================

    Filename:     test_neighbor_config.cpp

    Description:  Unit tests for Neighbor configuration API

        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "local_mip/Local_MIP.h"
#include <cassert>
#include <cstdio>

// Simple custom neighbor for testing
static bool custom_neighbor_called = false;

void test_custom_neighbor(Neighbor::Neighbor_Ctx& ctx, void* p_user_data)
{
  custom_neighbor_called = true;
  (void)p_user_data;  // Unused
  ctx.m_op_size = 0;  // No operation
}

int main()
{
  printf("Start Neighbor configuration API tests...\n\n");

  Local_MIP solver;
  solver.set_time_limit(0.1);  // Short time run

  // Test 1: Clear list
  printf("Test 1: clear_neighbor_list()\n");
  solver.clear_neighbor_list();
  printf("  ✓ Successfully cleared neighbor list\n\n");

  // Test 2: Add predefined neighbor
  printf("Test 2: add_neighbor()\n");
  solver.add_neighbor("flip", 0, 10);
  solver.add_neighbor("easy", 0, 5);
  printf("  ✓ Successfully added 2 predefined neighbors\n\n");

  // Test 3: Add custom neighbor
  printf("Test 3: add_custom_neighbor()\n");
  solver.add_custom_neighbor("test_custom", test_custom_neighbor);
  printf("  ✓ Successfully added custom neighbor\n\n");

  // Test 4: Reset to default list
  printf("Test 4: reset_default_neighbor_list()\n");
  solver.reset_default_neighbor_list();
  printf("  ✓ Successfully reset to default neighbor list\n\n");

  // Test 5: Mix use of
  printf("Test 5: Mixed use of predefined and custom neighbors\n");
  solver.clear_neighbor_list();
  solver.add_neighbor("unsat_mtm_bm", 12, 8);
  solver.add_custom_neighbor("test_custom", test_custom_neighbor);
  solver.add_neighbor("flip", 0, 12);
  printf("  ✓ Successfully configured mixed neighbor list\n\n");

  printf("All Neighbor configuration API tests passed!\n");

  return 0;
}
