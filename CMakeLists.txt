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
    # include/buffer.h
    include/csv.h
    include/dictionary.h
    include/error.h
    include/frame.h
    include/huffman.h
    include/image.h
    include/list.h
    # include/png.h
    include/rle.h
    # include/stack.h
    include/vector.h
    # include/zip.h
    src/analysis/vector.c
    src/compression/rle.c
    src/compression/huffman.c
    src/data/array2d.c
    src/data/dictionary.c
    src/data/list.c
    # src/data/stack.c
    src/file/bmp.c
    src/file/csv.c
    # src/file/png.c
    src/graphics/image.c
    src/graphics/frame.c
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
add_executable(frame_test test/graphics/frame_test.c)
target_link_libraries(frame_test PRIVATE breakdown)

add_executable(image_test test/graphics/image_test.c)
target_link_libraries(image_test PRIVATE breakdown)
enable_testing()
add_test(NAME IMAGE_NO_FILE COMMAND ./image_test)
set_tests_properties(IMAGE_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME IMAGE_LENA COMMAND ./image_test ../test/graphics/lena.bmp)

add_executable(vector_test test/analysis/vector_test.c)
target_link_libraries(vector_test PRIVATE breakdown)
add_test(NAME VECTOR_NO_FILE COMMAND ./vector_test)
set_tests_properties(VECTOR_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME VECTOR_TEST_CSV COMMAND ./vector_test ../test/analysis/test_vect.csv)

add_executable(rle_test test/compression/rle_test.c)
target_link_libraries(rle_test PRIVATE breakdown)

add_executable(huffman_test test/compression/huffman_test.c)
target_link_libraries(huffman_test PRIVATE breakdown)

add_executable(list_test test/data/list_test.c)
target_link_libraries(list_test PRIVATE breakdown)

add_executable(dictionary_test test/data/dictionary_test.c)
target_include_directories(dictionary_test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(dictionary_test PRIVATE breakdown)