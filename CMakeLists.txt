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
include(CheckCCompilerFlag)

set(BINREC_ENABLE_ASSERT TRUE)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB SOURCES src/*.c)
add_library(binrec STATIC ${SOURCES})

target_include_directories(binrec PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(BINREC_ENABLE_ASSERT)
    target_compile_definitions(binrec PRIVATE ENABLE_ASSERT)
endif()
