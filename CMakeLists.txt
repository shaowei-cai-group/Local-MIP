cmake_minimum_required(VERSION 3.15)
project(Local-MIP VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(Threads REQUIRED)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.cc")
list(REMOVE_ITEM LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/utils/main.cpp")

add_library(LocalMIP STATIC ${LIB_SOURCES})
add_library(LocalMIP::core ALIAS LocalMIP)

target_include_directories(LocalMIP PUBLIC
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/local_mip
)

target_compile_definitions(LocalMIP PUBLIC $<$<CONFIG:Debug>:DEBUG>)

target_compile_options(LocalMIP PUBLIC
  -fPIC
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>
)

target_link_libraries(LocalMIP PUBLIC
  Threads::Threads
)

add_executable(${PROJECT_NAME} src/utils/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE LocalMIP::core)

enable_testing()
add_subdirectory(tests)
