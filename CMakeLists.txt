cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

if(NOT GENERATOR_IS_MULTI_CONFIG)
  # Default to Release if not set
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
endif()


set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON) # let vcpkg install dependencies with install()

project(celestite
LANGUAGES CXX
VERSION 0.1.0)

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(CLST_OPTIMIZED_X86_64 "Enable optimized x86-64 builds, targetting x86-64-v3 or equivalent." OFF)
option(CLST_BUILD_CLI_APPS "Build command line apps." ON)
option(CLST_ENABLE_TESTS "Build and run tests." ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prevent full source file path.
# Should also be set for (statically) linked libraries.
# no full path should be the default for MSVC (i.e. no /FC)
# See https://gitlab.kitware.com/cmake/cmake/-/merge_requests/2306
add_compile_options("$<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>")

# Set source encoding to UTF-8
add_compile_options("$<IF:$<CXX_COMPILER_ID:MSVC>,/source-charset:utf-8,-finput-charset=utf-8>")

# Optimized x86-64 build
if(CLST_OPTIMIZED_X86_64 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-march=x86-64-v3" COMPILER_SUPPORTS_MARCH_X86_64_V3)
  add_compile_options($<IF:$<CXX_COMPILER_ID:MSVC>,/arch:AVX2,$<GENEX_EVAL:$<IF:$<BOOL:${COMPILER_SUPPORTS_MARCH_X86_64_V3}>,-march=x86-64-v3,-march=haswell>>>)
endif()

# add some warning flags
add_compile_options("$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-pedantic>")

# MSVC conformance mode
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)
# Enable updated __cplusplus macro for MSVC
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
# Enum type conformance options for MSVC. Can break ABI/API!!!
# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:enumEncoding>") # enum NTTP mangling
# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:enumTypes>")    # enum underlying type

# Do not export symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

# Define feature test macros for POSIX
if(UNIX)
add_compile_definitions(_POSIX_C_SOURCE=200809L)
if(APPLE)
add_compile_definitions(_DARWIN_C_SOURCE=1)
endif()
endif()

# compile-time log level
set(CLST_LOG_LEVEL DEBUG CACHE STRING "Compile-time log level for celestite.")
add_compile_definitions("CLST_LOG_LEVEL=CLST_LOG_LEVEL_${CLST_LOG_LEVEL}")

#-------- Version & Build info
include(cmake/git-version.cmake)

set(GENERATED_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(cmake/version.cpp.in ${GENERATED_SOURCE_DIR}/version.cpp @ONLY)
#--------

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(CLST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CLST_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CLST_CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/tests/tests.cmake)

add_subdirectory(third_party/gpmf-parser)
add_subdirectory(src/common)
add_subdirectory(src/clst)
if(CLST_BUILD_CLI_APPS)
add_subdirectory(src/apps)
endif()

if(CLST_ENABLE_TESTS)
include(CTest)
add_subdirectory(test)
endif()

#include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GIT_DESC}-${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
if(CMAKE_BUILD_TYPE)
STRING(APPEND CPACK_PACKAGE_FILE_NAME "-${CMAKE_BUILD_TYPE}")
endif()

#set(CPACK_THREADS 0)
set(CPACK_GENERATOR "TZST")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES ON)
include(CPack)
