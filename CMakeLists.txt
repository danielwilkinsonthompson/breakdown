# =============================================================================
#                              breakdown cmake
# -----------------------------------------------------------------------------
# build the breakdown library
#
# © Daniel Wilkinson-Thompson 2023
# daniel@wilkinson-thompson.com
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.7)
project(breakdown)

# -----------------------------------------------------------------------------
# minifb
# -----------------------------------------------------------------------------
add_subdirectory(lib/minifb lib)

# -----------------------------------------------------------------------------
# breakdown
# -----------------------------------------------------------------------------
set(file_lib
    include/bmp.h
    src/file/bmp/bmp.c
)
add_library(breakdown STATIC ${file_lib})
target_include_directories(breakdown PUBLIC "${CMAKE_INSTALL_INCLUDEDIR}")
target_link_libraries(breakdown PUBLIC minifb)

# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------
add_executable(bmp test/file/bmp/bmp_test.c)
target_link_libraries(bmp PRIVATE breakdown)