file(GLOB clst_test_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
"*.c"
"*.cpp"
)

create_test_sourcelist(clst_test_driver_sources test_clst.cpp ${clst_test_sources})

add_executable(test_clst ${clst_test_driver_sources})
target_link_libraries(test_clst PRIVATE clst)

foreach(test ${clst_test_sources})
  get_filename_component(test_name ${test} NAME_WE)
  add_test(NAME ${test_name} COMMAND test_clst ${test_name})
endforeach ()
