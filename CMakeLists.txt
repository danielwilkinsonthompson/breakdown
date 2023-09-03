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
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
    add_definitions(-DDEBUG)
endif()
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
    include/bits.h
    include/buffer.h
    include/csv.h
    include/deflate.h
    include/dictionary.h
    include/draw.h
    include/error.h
    include/frame.h
    include/gui.h
    include/gz.h
    include/hexdump.h
    include/image.h
    include/layer.h
    include/list.h
    include/plot.h
    include/png.h
    include/rle.h
    include/stream.h
    include/vector.h
    include/zlib.h
    src/analysis/vector.c
    src/compression/deflate.c
    src/compression/rle.c
    src/compression/zlib.c
    src/data/array2d.c
    src/data/buffer.c
    src/data/dictionary.c
    src/data/list.c
    src/data/stream.c
    src/ecc/crc.c
    src/file/bmp.c
    src/file/csv.c
    src/file/gz.c
    src/file/png.c
    src/graphics/draw.c
    src/graphics/image.c
    src/graphics/frame.c
    src/graphics/gui.c
    src/graphics/layer.c
    src/graphics/plot.c
    src/system/bits.c
    src/system/error.c
    src/system/hexdump.c
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
# graphics/frame
add_executable(frame_test test/graphics/frame_test.c)
target_link_libraries(frame_test PRIVATE breakdown)

# data/dictionary
add_executable(dictionary_test test/data/dictionary_test.c)
target_include_directories(dictionary_test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(dictionary_test PRIVATE breakdown)

# data/stream
add_executable(stream_test test/data/stream_test.c)
target_link_libraries(stream_test PRIVATE breakdown)

# compression/zlib
add_executable(zlib_test test/compression/zlib_test.c)
target_link_libraries(zlib_test PRIVATE breakdown)

# compression/huffman
# add_executable(huffman_test test/compression/huffman_test.c)
# target_link_libraries(huffman_test PRIVATE breakdown)

# compression/gz
add_executable(gz_test test/compression/gz_test.c)
target_link_libraries(gz_test PRIVATE breakdown)

# graphics/image
add_executable(image_test test/graphics/image_test.c)
target_link_libraries(image_test PRIVATE breakdown)
enable_testing()
add_test(NAME IMAGE_NO_FILE COMMAND ./image_test)
set_tests_properties(IMAGE_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME IMAGE_LENA COMMAND ./image_test ../test/graphics/lena.bmp)

# data/list
add_executable(list_test test/data/list_test.c)
target_link_libraries(list_test PRIVATE breakdown)

# file/png
add_executable(png_test test/file/png_test.c)
target_link_libraries(png_test PRIVATE breakdown)

# compression/rle
add_executable(rle_test test/compression/rle_test.c)
target_link_libraries(rle_test PRIVATE breakdown)

# analysis/vector
add_executable(vector_test test/analysis/vector_test.c)
target_link_libraries(vector_test PRIVATE breakdown)
add_test(NAME VECTOR_NO_FILE COMMAND ./vector_test)
set_tests_properties(VECTOR_NO_FILE PROPERTIES PASS_REGULAR_EXPRESSION "No file specified")
add_test(NAME VECTOR_TEST_CSV COMMAND ./vector_test ../test/analysis/test_vect.csv)
