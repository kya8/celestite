find_package(Boost CONFIG REQUIRED) # circular_buffer, header-only

file(GLOB_RECURSE clst_common_sources CONFIGURE_DEPENDS *.h *.hpp *.cpp *.c)
list(FILTER clst_common_sources EXCLUDE REGEX "(^|/)_")

add_library(clst_common STATIC ${clst_common_sources} ${GENERATED_SOURCE_DIR}/version.cpp)
add_library(clst::common ALIAS clst_common)

target_link_libraries(clst_common PRIVATE Threads::Threads PUBLIC Boost::headers)
target_include_directories(clst_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(clst_common PRIVATE CLST_BUILD_TYPE=\"$<CONFIG>\")
