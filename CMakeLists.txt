# Disallow in-source builds
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
")
endif()

cmake_minimum_required(VERSION 3.19.0 FATAL_ERROR)

project(celestite
        LANGUAGES CXX
        VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CLST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/clst/)

set(ALL_HEADER_FILES
meta/sequence.h meta/traits.h meta/meta_list.h
numerical/exceptions.h numerical/constants.h numerical/math.h numerical/num_cast.h
strings/fixed_string.h strings/typed_string.h strings/type_name.h
concurrency/simple_thread_pool.h concurrency/tpool.h concurrency/exceptions.h
util/types/ints.h util/timer.h util/char_num.h util/endian.h util/endian_rw.h util/tuples.h util/utilities.h
util/throwing_ptr.h
common/exceptions.h
stream/binary_stream.h stream/file_stream.h stream/lfs.h stream/cursor.h
iter/enumerate.h iter/zip.h iter/nd.h
container/ring_buffer.h
)
list(TRANSFORM ALL_HEADER_FILES PREPEND ${CLST_ROOT})

set(ALL_SOURCE_FILES
concurrency/tpool.cpp
util/char_num.cpp
stream/binary_stream.cpp stream/file_stream.cpp stream/cursor.cpp
)
list(TRANSFORM ALL_SOURCE_FILES PREPEND ${CLST_ROOT})


##### version #####
set(GIT_DESC Unknown)
set(GIT_BRANCH Unknown)
set(COMMIT_DATE Unknown)
set(COMMIT_HASH Unknown)

if(CMAKE_CXX_COMPILER_ID)
  set(COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
else()
  set(COMPILER_NAME Unknown)
endif()
if(CMAKE_CXX_COMPILER_VERSION)
  set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
else()
  set(COMPILER_VERSION Unknown)
endif()

find_package(Git)
if(Git_FOUND)
  # check if we're in a valid git repo
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree OUTPUT_VARIABLE INSIDE_GIT_REPO OUTPUT_STRIP_TRAILING_WHITESPACE)
  if("${INSIDE_GIT_REPO}" STREQUAL "true")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --broken OUTPUT_VARIABLE GIT_DESC OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cs OUTPUT_VARIABLE COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    message(WARNING "Not in a git repository.")
  endif()
else()
  message(WARNING "Git was not found.")
endif()

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(${CLST_ROOT}/version.cpp.in ${GENERATED_DIR}/version.cpp @ONLY)
list(APPEND ALL_SOURCE_FILES ${GENERATED_DIR}/version.cpp)
##### END version


add_library(celestite STATIC
${ALL_HEADER_FILES}
${ALL_SOURCE_FILES}
)

target_include_directories(celestite PUBLIC .)


##### Tests #####
enable_testing()
find_package(GTest REQUIRED)

add_executable(test_seq test/sequence.cpp)
target_link_libraries(test_seq PRIVATE celestite GTest::gtest_main)
add_executable(test_utils test/utils.cpp)
target_link_libraries(test_utils PRIVATE celestite GTest::gtest_main)
add_executable(test_num_cast test/num_cast.cpp)
target_link_libraries(test_num_cast PRIVATE celestite GTest::gtest_main)
add_executable(test_iter test/iter.cpp)
target_link_libraries(test_iter PRIVATE celestite GTest::gtest_main)
add_executable(test_common test/common.cpp)
target_link_libraries(test_common PRIVATE celestite GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_seq)
gtest_discover_tests(test_utils)
gtest_discover_tests(test_num_cast)
gtest_discover_tests(test_iter)
gtest_discover_tests(test_common)
