cmake_minimum_required(VERSION 3.15...3.27)
project(localmip_py LANGUAGES CXX)

# Find pybind11 (expects it installed or discoverable via CMAKE_PREFIX_PATH)
find_package(pybind11 REQUIRED)

# Disable IPO/LTO to avoid toolchain-induced zero-sized outputs
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(PYBIND11_LTO_CXX_FLAGS "" CACHE STRING "" FORCE)
set(PYBIND11_LTO_C_FLAGS "" CACHE STRING "" FORCE)

# Local-MIP project root (assumes this CMakeLists lives in python-bindings/ under repo root)
set(LOCALMIP_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(LOCALMIP_BUILD_DIR "${LOCALMIP_ROOT}/build")
set(LOCALMIP_INCLUDE_DIR "${LOCALMIP_ROOT}/src")

# Static library path (built by ./build.sh release|debug)
set(LOCALMIP_LIB "${LOCALMIP_BUILD_DIR}/libLocalMIP.a")
if(NOT EXISTS ${LOCALMIP_LIB})
  message(FATAL_ERROR "Local-MIP static library not found at ${LOCALMIP_LIB}. Build project root first with ./build.sh release|debug.")
endif()

pybind11_add_module(localmip_py MODULE local_mip_py.cpp)
target_include_directories(localmip_py PRIVATE ${LOCALMIP_INCLUDE_DIR})
target_link_libraries(localmip_py PRIVATE ${LOCALMIP_LIB} pthread)
target_compile_features(localmip_py PRIVATE cxx_std_20)
set_target_properties(localmip_py PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

# Produce a plain .so (no python prefix/suffix mangling already handled by pybind11_add_module)
set_target_properties(localmip_py PROPERTIES
  OUTPUT_NAME "localmip_py"
)
