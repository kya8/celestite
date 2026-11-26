file(GLOB_RECURSE test_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS *.cpp)
list(FILTER test_sources EXCLUDE REGEX "(^|/)_")

create_test_sourcelist(driver_sources clst_tests_driver.cpp ${test_sources})

add_executable(test_clst ${driver_sources})
# Exercise internal parsers and streams in both static and shared builds.
target_sources(test_clst PRIVATE
    ${CLST_SRC_DIR}/clst/gpmf.cpp
    ${CLST_SRC_DIR}/clst/gpmf_timing.cpp
    ${CLST_SRC_DIR}/clst/mp4/mp4.cpp
    ${CLST_SRC_DIR}/clst/streams/BinaryFileStream.cpp
    ${CLST_SRC_DIR}/clst/streams/NativeBinaryFileStream.cpp
)
target_include_directories(test_clst PRIVATE ${CLST_SRC_DIR})
target_link_libraries(test_clst PRIVATE clst::clst)

foreach(test ${test_sources})
  get_filename_component(test_name ${test} NAME_WE)
  add_test(NAME ${test_name} COMMAND test_clst ${test_name})
endforeach()
