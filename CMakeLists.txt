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
meta/sequence.h meta/traits.h meta/type_holder.h meta/utils.h
numerical/constants.h
strings/fixed_string.h strings/typed_string.h
util/types/ints.h util/timer.h
)
list(TRANSFORM ALL_HEADER_FILES PREPEND ${CLST_ROOT})

set(ALL_SOURCE_FILES "")


##### version #####
set(GIT_DESC Unknown)
set(GIT_BRANCH Unknown)
set(COMMIT_DATE Unknown)

find_package(Git)
if(Git_FOUND)
  # check if we're in a valid git repo
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree OUTPUT_VARIABLE INSIDE_GIT_REPO OUTPUT_STRIP_TRAILING_WHITESPACE)
  if("${INSIDE_GIT_REPO}" STREQUAL "true")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --broken OUTPUT_VARIABLE GIT_DESC OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cs OUTPUT_VARIABLE COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    message(WARNING "Not in a git repository.")
  endif()
else()
  message(WARNING "Git was not found.")
endif()

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(version.cpp.in ${GENERATED_DIR}/version.cpp @ONLY)
list(APPEND ALL_SOURCE_FILES ${GENERATED_DIR}/version.cpp)
##### END version


add_library(celestite
${ALL_HEADER_FILES}
${ALL_SOURCE_FILES}
)

target_include_directories(celestite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
