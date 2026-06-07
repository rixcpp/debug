cmake_minimum_required(VERSION 3.20)

project(rix_debug
  VERSION 0.1.0
  DESCRIPTION "Debug printing, formatting, logging, and inspection utilities for Rix"
  LANGUAGES CXX
)

option(RIX_DEBUG_BUILD_EXAMPLES "Build rix/debug examples" ON)
option(RIX_DEBUG_BUILD_TESTS "Build rix/debug tests" ON)

add_library(rix_debug INTERFACE)
add_library(rix::debug ALIAS rix_debug)

target_compile_features(rix_debug INTERFACE cxx_std_20)

target_include_directories(rix_debug
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if(RIX_DEBUG_BUILD_EXAMPLES)
    add_subdirectory(examples)
  endif()

  if(RIX_DEBUG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()
