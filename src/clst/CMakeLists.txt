option(CLST_ENABLE_ANY "Include std::any" OFF)

find_package(Protobuf REQUIRED)
find_package(Threads)
find_package(Eigen3 CONFIG REQUIRED)

file(GLOB_RECURSE clst_sources CONFIGURE_DEPENDS
    ${CLST_PUBLIC_INCLUDE_DIR}/clst/*.hpp
    ${CLST_PUBLIC_INCLUDE_DIR}/clst/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.proto
)
list(FILTER clst_sources EXCLUDE REGEX "(^|/)_")

add_library(clst ${clst_sources})
add_library(clst::clst ALIAS clst)

set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
protobuf_generate(
  TARGET clst
  LANGUAGE cpp
  PROTOC_OUT_DIR ${PROTO_GEN_DIR}
)

target_include_directories(clst
PUBLIC
    $<BUILD_INTERFACE:${CLST_PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ${PROTO_GEN_DIR}
    ${CLST_SRC_DIR}
)

target_link_libraries(clst
PRIVATE gpmf-parser protobuf::libprotobuf clst::common
PUBLIC Eigen3::Eigen # Insta360 transforms
)

# export macros
target_compile_definitions(clst PRIVATE CLST_SOURCE)
if(BUILD_SHARED_LIBS)
target_compile_definitions(clst PUBLIC CLST_SHARED_LIB)
endif()

# Workaround a GCC/libstdc++ bug, triggered by protobuf.
# Ref:
# https://github.com/protocolbuffers/protobuf/issues/6479
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55394
# (Only for linking to static protobuf library.)
if(Threads_FOUND)
target_link_libraries(clst PRIVATE Threads::Threads)
endif()

if(CLST_ENABLE_ANY)
target_compile_definitions(clst PUBLIC CLST_ENABLE_ANY)
endif()

install(TARGETS clst)
install(DIRECTORY ${CLST_PUBLIC_INCLUDE_DIR}/clst TYPE INCLUDE)
