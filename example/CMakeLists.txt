cmake_minimum_required(VERSION 3.15)
project(LocalMIP_Examples)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compilation options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Find LocalMIP static library
# Prioritize local lib directory, fallback to main project build directory
find_library(LOCALMIP_LIB
    NAMES LocalMIP libLocalMIP.a
    PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/../build
    REQUIRED
)

# Set header file paths
# Prioritize local include directory, fallback to main project src directory
if(EXISTS ${CMAKE_SOURCE_DIR}/include)
    set(LOCALMIP_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/include
    )
else()
    set(LOCALMIP_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/../src/local_mip
    )
endif()

# Find pthread (LocalMIP unique dependency)
find_package(Threads REQUIRED)

# simple-api example
add_executable(simple_api_demo
    simple-api/simple_api.cpp
)

target_include_directories(simple_api_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(simple_api_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# start-callback example
add_executable(start_callback_demo
    start-callback/start_callback.cpp
)

target_include_directories(start_callback_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(start_callback_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# restart-callback example
add_executable(restart_callback_demo
    restart-callback/restart_callback.cpp
)

target_include_directories(restart_callback_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(restart_callback_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# weight-callback example
add_executable(weight_callback_demo
    weight-callback/weight_callback.cpp
)

target_include_directories(weight_callback_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(weight_callback_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# scoring-lift example
add_executable(lift_degree_demo
    scoring-lift/lift_degree.cpp
)

target_include_directories(lift_degree_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(lift_degree_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# scoring-neighbor example
add_executable(neighbor_random_demo
    scoring-neighbor/neighbor_random.cpp
)

target_include_directories(neighbor_random_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(neighbor_random_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# neighbor-config example
add_executable(neighbor_config_demo
    neighbor-config/neighbor_config.cpp
)

target_include_directories(neighbor_config_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(neighbor_config_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# neighbor-userdata example
add_executable(neighbor_userdata_demo
    neighbor-userdata/neighbor_userdata.cpp
)

target_include_directories(neighbor_userdata_demo PRIVATE
    ${LOCALMIP_INCLUDE_DIRS}
)

target_link_libraries(neighbor_userdata_demo PRIVATE
    ${LOCALMIP_LIB}
    Threads::Threads
)

# Set output directories
set_target_properties(simple_api_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/simple-api
)

set_target_properties(start_callback_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/start-callback
)

set_target_properties(restart_callback_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/restart-callback
)

set_target_properties(weight_callback_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/weight-callback
)

set_target_properties(lift_degree_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/scoring-lift
)

set_target_properties(neighbor_random_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/scoring-neighbor
)

set_target_properties(neighbor_config_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/neighbor-config
)

set_target_properties(neighbor_userdata_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/neighbor-userdata
)

message(STATUS "LocalMIP library: ${LOCALMIP_LIB}")
message(STATUS "LocalMIP include directories: ${LOCALMIP_INCLUDE_DIRS}")
