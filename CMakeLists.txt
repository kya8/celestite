cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

if(NOT GENERATOR_IS_MULTI_CONFIG)
  # Default to Release if not set
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
endif()

project(celestite LANGUAGES CXX)

option(CLST_ENABLE_TESTS "Build tests." ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Do not export symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

file(GLOB_RECURSE clst_sources CONFIGURE_DEPENDS
"clst/*.c"
"clst/*.cpp"
"clst/*.h"
"clst/*.hpp"
)

add_library(clst STATIC ${clst_sources})
target_include_directories(clst PUBLIC clst)
target_compile_definitions(clst PRIVATE CLST_SOURCE)

get_target_property(clst_lib_type clst TYPE)
# Currently unused
if(clst_lib_type STREQUAL "SHARED_LIBRARY")
target_compile_definitions(clst PUBLIC CLST_SHARED_LIB)
endif()

# Set source encoding to UTF-8
target_compile_options(clst PRIVATE "$<IF:$<CXX_COMPILER_ID:MSVC>,/source-charset:utf-8,-finput-charset=utf-8>")

# Prevent full source file path.
# See https://gitlab.kitware.com/cmake/cmake/-/merge_requests/2306
target_compile_options(clst PRIVATE "$<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>")

# Warnings
target_compile_options(clst PRIVATE "$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-pedantic>")

# MSVC conformance mode
target_compile_options(clst PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/permissive->)
# Enable updated __cplusplus macro for MSVC.
# Might be required by public headers.
target_compile_options(clst PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)

# Define feature test macros for Unices.
# Some public headers might require this, but we define them separately.
if(UNIX)
target_compile_definitions(clst PRIVATE _POSIX_C_SOURCE=200809L)
if(APPLE)
target_compile_definitions(clst PRIVATE _DARWIN_C_SOURCE=1) # Required to expose darwin-specific functions.
endif()
endif()

# set(THREADS_PREFER_PTHREAD_FLAG TRUE)
# find_package(Threads REQUIRED)


##### Tests
if(CLST_ENABLE_TESTS)
include(CTest)
add_subdirectory(test)
endif()
