file(GLOB_RECURSE test_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS *.cpp)
list(FILTER test_sources EXCLUDE REGEX "(^|/)_")

create_test_sourcelist(driver_sources clst_tests_driver.cpp ${test_sources})

add_executable(test_clst ${driver_sources})
target_link_libraries(test_clst PRIVATE clst::clst)

foreach(test ${test_sources})
  get_filename_component(test_name ${test} NAME_WE)
  add_test(NAME ${test_name} COMMAND test_clst ${test_name})
endforeach()
