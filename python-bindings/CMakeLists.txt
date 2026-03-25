cmake_minimum_required(VERSION 3.15...3.27)

# Set minimum policy version to allow older pybind11 CMake files
cmake_policy(VERSION 3.5)

project(localmip_py LANGUAGES CXX)

# Suppress deprecation warnings from pybind11's legacy Python finding modules
if(POLICY CMP0148)
  cmake_policy(SET CMP0148 OLD)
endif()

# Find pybind11 (expects it installed or discoverable via CMAKE_PREFIX_PATH)
find_package(Threads REQUIRED)
find_package(pybind11 REQUIRED)

# Disable IPO/LTO to avoid toolchain-induced zero-sized outputs
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(PYBIND11_LTO_CXX_FLAGS "" CACHE STRING "" FORCE)
set(PYBIND11_LTO_C_FLAGS "" CACHE STRING "" FORCE)

# Prefer bundled core sources in source distributions; fall back to the repo root in development checkouts.
set(LOCALMIP_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/localmip_core/src")
  set(LOCALMIP_ROOT "${CMAKE_CURRENT_LIST_DIR}/localmip_core")
endif()
set(LOCALMIP_INCLUDE_DIR "${LOCALMIP_ROOT}/src")
file(GLOB_RECURSE LOCALMIP_SOURCES CONFIGURE_DEPENDS
  "${LOCALMIP_ROOT}/src/*.cpp"
  "${LOCALMIP_ROOT}/src/*.cc"
)
list(REMOVE_ITEM LOCALMIP_SOURCES "${LOCALMIP_ROOT}/src/utils/main.cpp")

add_library(LocalMIP STATIC ${LOCALMIP_SOURCES})
target_include_directories(LocalMIP PUBLIC
  ${LOCALMIP_INCLUDE_DIR}
  ${LOCALMIP_ROOT}/src/local_mip
)
target_compile_features(LocalMIP PUBLIC cxx_std_20)
target_compile_definitions(LocalMIP PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_compile_options(LocalMIP PUBLIC
  -fPIC
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>
)
target_link_libraries(LocalMIP PUBLIC Threads::Threads)

pybind11_add_module(localmip_py MODULE local_mip_py.cpp)
target_include_directories(localmip_py PRIVATE ${LOCALMIP_INCLUDE_DIR})
target_link_libraries(localmip_py PRIVATE LocalMIP)
target_compile_features(localmip_py PRIVATE cxx_std_20)
set_target_properties(localmip_py PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

# Use parallel LTO to suppress warnings when linking against LTO-compiled static library
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(localmip_py PRIVATE -flto=auto)
endif()

# Produce a plain .so (no python prefix/suffix mangling already handled by pybind11_add_module)
set_target_properties(localmip_py PROPERTIES
  OUTPUT_NAME "localmip_py"
)

install(TARGETS localmip_py LIBRARY DESTINATION .)
