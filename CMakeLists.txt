#
# libbinrec: a recompiling translator for machine code
# Copyright (c) 2016 Andrew Church <achurch@achurch.org>
#
# This software may be copied and redistributed under certain conditions;
# see the file "COPYING" in the source code distribution for details.
# NO WARRANTY is provided with this software.
#

# This CMake control file is intended for embedding libbinrec within other
# programs; it may not be useful for installing a standalone copy of the
# library.

cmake_minimum_required(VERSION 2.8)
project(binrec C CXX)
include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(BINREC_ENABLE_ASSERT TRUE CACHE BOOL "Enable basic assertion checks")
set(BINREC_ENABLE_OPERAND_SANITY_CHECKS TRUE CACHE BOOL "Enable code generation assertion checks")
set(BINREC_ENABLE_RTL_DEBUG_OPTIMIZE TRUE CACHE BOOL "Enable debug output from optimization passes")
set(BINREC_BUILD_TESTS FALSE CACHE BOOL "Build tests")

if(BINREC_ENABLE_ASSERT)
   add_definitions(-DENABLE_ASSERT)
endif()

if(BINREC_ENABLE_OPERAND_SANITY_CHECKS)
   add_definitions(-DENABLE_OPERAND_SANITY_CHECKS)
endif()

if(BINREC_ENABLE_RTL_DEBUG_OPTIMIZE)
   add_definitions(-DRTL_DEBUG_OPTIMIZE)
endif()

add_definitions(-DVERSION="0.2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# Build binrec library
file(GLOB SOURCES src/*.c src/*/*.c)
add_library(binrec STATIC ${SOURCES})
target_include_directories(binrec PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(BINREC_BUILD_TESTS)
   # Enable ctest
   enable_testing()

   # Build shared test library
   add_library(testlib STATIC
      tests/common.c
      tests/common.h
      tests/execute.c
      tests/execute.h
      tests/log-capture.c
      tests/log-capture.h
      tests/mem-wrappers.c
      tests/mem-wrappers.h
      tests/ppc-lut.c
      tests/ppc-lut.h)

   # Build tests
   file(GLOB TEST_SOURCES_FULL tests/*/*/*.c tests/*/*.c tests/api/binrec++.cc)
   foreach(test_src ${TEST_SOURCES_FULL})
      # Parse the test file path
      get_filename_component(test_name ${test_src} NAME_WE)
      get_filename_component(test_dir1 ${test_src} DIRECTORY)
      get_filename_component(test_dir1_name ${test_dir1} NAME)
      get_filename_component(test_dir2 ${test_dir1} DIRECTORY)
      get_filename_component(test_dir2_name ${test_dir2} NAME)

      if(${test_dir2_name} STREQUAL "tests")
         # tests/dir1/test_name
         set(test_target ${test_dir1_name}_${test_name})
         set(test_output_dir ${CMAKE_BINARY_DIR}/tests/${test_dir1_name})
      else()
         # tests/dir2/dir1/test_name
         set(test_target ${test_dir2_name}_${test_dir1_name}_${test_name})
         set(test_output_dir ${CMAKE_BINARY_DIR}/tests/${test_dir2_name}/${test_dir1_name})
      endif()

      # Add the test executable target
      add_executable(${test_target} ${test_src})
      target_link_libraries(${test_target} binrec testlib)
      set_target_properties(${test_target} PROPERTIES
         RUNTIME_OUTPUT_DIRECTORY ${test_output_dir}
         OUTPUT_NAME ${test_name})

      # Register with ctest
      add_test(${test_target} ${test_output_dir}/${test_name})
   endforeach()
endif()
