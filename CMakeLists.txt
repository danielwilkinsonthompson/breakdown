# =============================================================================
#                                   breakdown
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
    include/array2d.h    
    include/bmp.h
    include/csv.h
    include/vector.h
    include/image.h
    src/data/array2d.c
    src/file/bmp.c
    src/file/csv.c
    src/analysis/vector.c
    src/graphics/image.c
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
add_executable(image_test test/graphics/image_test.c)
target_link_libraries(image_test PRIVATE breakdown)
install(TARGETS image_test EXPORT image_test RUNTIME DESTINATION bin)

add_executable(bmp_test test/file/bmp/bmp_test.c)
target_link_libraries(bmp_test PRIVATE breakdown)
install(TARGETS bmp_test EXPORT bmp_test RUNTIME DESTINATION bin)
enable_testing()
add_test(NAME BMP_NO_FILE COMMAND ./bmp_test)
set_tests_properties(BMP_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME BMP_LENA COMMAND ./bmp_test ../test/file/bmp/lena.bmp)

add_executable(vector_test test/analysis/vector_test.c)
target_link_libraries(vector_test PRIVATE breakdown)
install(TARGETS vector_test EXPORT vector_test RUNTIME DESTINATION bin)
add_test(NAME VECTOR_NO_FILE COMMAND ./vector_test)
set_tests_properties(VECTOR_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME VECTOR_TEST_CSV COMMAND ./vector_test ../test/analysis/test_vect.csv)