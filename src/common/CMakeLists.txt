find_package(Boost CONFIG REQUIRED) # circular_buffer, header-only

file(GLOB_RECURSE clstcommon_sources CONFIGURE_DEPENDS *.h *.hpp *.cpp *.c)
list(FILTER clstcommon_sources EXCLUDE REGEX "(^|/)_")

add_library(clstcommon STATIC ${clstcommon_sources} ${GENERATED_SOURCE_DIR}/version.cpp)
add_library(clst::common ALIAS clstcommon)

target_link_libraries(clstcommon PRIVATE Threads::Threads PUBLIC Boost::headers)
target_include_directories(clstcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(clstcommon PRIVATE CLST_BUILD_TYPE=\"$<CONFIG>\")
