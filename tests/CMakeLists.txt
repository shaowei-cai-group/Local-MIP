# =============================================================================
# Automated Instance Tests: Test all .mps files in test-set/
# =============================================================================
file(GLOB TEST_INSTANCES CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/test-set/*.mps")
foreach(instance ${TEST_INSTANCES})
  get_filename_component(name ${instance} NAME_WE)
  add_test(NAME solver_${name}
    COMMAND $<TARGET_FILE:Local-MIP> -i ${instance} -t 1
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endforeach()

# =============================================================================
# Test Path Definitions
# =============================================================================
set(TEST_UTILS_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}")
set(TEST_MPS_FILE "${PROJECT_SOURCE_DIR}/test-set/sct1.mps")
set(TEST_LP_FILE "${PROJECT_SOURCE_DIR}/test-set/sct1.lp")

# =============================================================================
# API Tests
# =============================================================================
add_executable(local_mip_api_test api/test_local_mip_api.cpp)
target_link_libraries(local_mip_api_test PRIVATE LocalMIP::core)
target_include_directories(local_mip_api_test PRIVATE ${TEST_UTILS_INCLUDE})
target_compile_definitions(local_mip_api_test PRIVATE
  TEST_MPS_PATH="${TEST_MPS_FILE}"
  TEST_LP_PATH="${TEST_LP_FILE}")

add_test(NAME api
  COMMAND $<TARGET_FILE:local_mip_api_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Callback Tests
# =============================================================================
add_executable(local_mip_callbacks_test callbacks/test_local_mip_callbacks.cpp)
target_link_libraries(local_mip_callbacks_test PRIVATE LocalMIP::core)
target_include_directories(local_mip_callbacks_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME callbacks
  COMMAND $<TARGET_FILE:local_mip_callbacks_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Constraint Type Recognition Tests
# =============================================================================
add_executable(constraint_recognition_test
  constraint_types/test_constraint_recognition.cpp)
target_link_libraries(constraint_recognition_test PRIVATE LocalMIP::core)
target_include_directories(constraint_recognition_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME constraint_recognition
  COMMAND $<TARGET_FILE:constraint_recognition_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Scoring Function Tests
# =============================================================================
add_executable(scoring_test scoring/test_scoring_functions.cpp)
target_link_libraries(scoring_test PRIVATE LocalMIP::core)
target_include_directories(scoring_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME scoring
  COMMAND $<TARGET_FILE:scoring_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Model_Manager Tests
# =============================================================================
add_executable(model_manager_test model/test_model_manager.cpp)
target_link_libraries(model_manager_test PRIVATE LocalMIP::core)
target_include_directories(model_manager_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME model_manager
  COMMAND $<TARGET_FILE:model_manager_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Reader Tests
# =============================================================================
add_executable(reader_test reader/test_readers.cpp)
target_link_libraries(reader_test PRIVATE LocalMIP::core)
target_include_directories(reader_test PRIVATE ${TEST_UTILS_INCLUDE})
target_compile_definitions(reader_test PRIVATE
  TEST_MPS_PATH="${TEST_MPS_FILE}"
  TEST_LP_PATH="${TEST_LP_FILE}")

add_test(NAME reader
  COMMAND $<TARGET_FILE:reader_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Move Operations Tests
# =============================================================================
add_executable(move_operations_test moves/test_move_operations.cpp)
target_link_libraries(move_operations_test PRIVATE LocalMIP::core)
target_include_directories(move_operations_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME move_operations
  COMMAND $<TARGET_FILE:move_operations_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# End-to-End Integration Tests
# =============================================================================
add_executable(integration_test integration/test_end_to_end.cpp)
target_link_libraries(integration_test PRIVATE LocalMIP::core)
target_include_directories(integration_test PRIVATE ${TEST_UTILS_INCLUDE})
target_compile_definitions(integration_test PRIVATE
  TEST_MPS_PATH="${TEST_MPS_FILE}")

add_test(NAME integration
  COMMAND $<TARGET_FILE:integration_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Neighbor Configuration API Tests
# =============================================================================
add_executable(neighbor_config_test neighbor_config/test_neighbor_config.cpp)
target_link_libraries(neighbor_config_test PRIVATE LocalMIP::core)
target_include_directories(neighbor_config_test PRIVATE ${TEST_UTILS_INCLUDE})

add_test(NAME neighbor_config
  COMMAND $<TARGET_FILE:neighbor_config_test>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Custom Test Targets
# =============================================================================
# Run all unit tests (excluding instance tests)
add_custom_target(unit-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    -R "^(api|callbacks|constraint_recognition|scoring|model_manager|reader|move_operations|neighbor_config)$$"
  DEPENDS local_mip_api_test
          local_mip_callbacks_test
          constraint_recognition_test
          scoring_test
          model_manager_test
          reader_test
          move_operations_test
          neighbor_config_test
  COMMENT "Running unit tests"
  VERBATIM)

# Run integration tests
add_custom_target(integration-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^integration$$"
  DEPENDS integration_test
  COMMENT "Running integration tests"
  VERBATIM)

# Run all tests (including instance tests)
add_custom_target(run-all-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS Local-MIP
          local_mip_api_test
          local_mip_callbacks_test
          constraint_recognition_test
          scoring_test
          model_manager_test
          reader_test
          move_operations_test
          neighbor_config_test
          integration_test
  COMMENT "Running all tests including instance tests"
  VERBATIM)

# Default run-tests target
add_custom_target(run-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    -R "^(api|callbacks|constraint_recognition|scoring|model_manager|reader|move_operations|neighbor_config|integration)$$"
  DEPENDS local_mip_api_test
          local_mip_callbacks_test
          constraint_recognition_test
          scoring_test
          model_manager_test
          reader_test
          move_operations_test
          neighbor_config_test
          integration_test
  COMMENT "Running all tests (unit + integration, excluding instance tests)"
  VERBATIM)

# =============================================================================
# Test Grouping
# =============================================================================
# Quick tests: run unit tests only, skip integration and instance tests
add_custom_target(quick-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    -R "^(api|callbacks|constraint_recognition|scoring|model_manager|reader|move_operations)$$"
  DEPENDS local_mip_api_test
          local_mip_callbacks_test
          constraint_recognition_test
          scoring_test
          model_manager_test
          reader_test
          move_operations_test
  COMMENT "Running quick unit tests"
  VERBATIM)

# Instance tests: run test-set instances only
add_custom_target(instance-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^solver_"
  DEPENDS Local-MIP
  COMMENT "Running instance tests on test-set"
  VERBATIM)
