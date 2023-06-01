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
option(BREAKDOWN_BUILD_TESTS "Build test apps" TRUE)


# -----------------------------------------------------------------------------
# minifb
# -----------------------------------------------------------------------------
add_subdirectory(lib/minifb lib)

# -----------------------------------------------------------------------------
# breakdown
# -----------------------------------------------------------------------------
set(breakdown_src
    include/bmp.h
    include/array2d.h
    src/file/bmp.c
    src/data/array2d.c
)
add_library(breakdown STATIC ${breakdown_src})
target_include_directories(breakdown PUBLIC "${CMAKE_INSTALL_INCLUDEDIR}")
target_link_libraries(breakdown PUBLIC minifb)
install(TARGETS breakdown EXPORT breakdown ARCHIVE)
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")
install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------
if(BREAKDOWN_BUILD_TESTS)
    add_executable(bmp test/file/bmp/bmp_test.c)
    target_link_libraries(bmp PRIVATE breakdown)
    install(TARGETS bmp EXPORT bmp RUNTIME DESTINATION bin)
endif()