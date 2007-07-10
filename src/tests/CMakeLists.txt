include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### renamedialogtest ###############


kde4_add_unit_test(renamedialogtest renamedialogtest.cpp)

target_link_libraries(renamedialogtest dolphinprivate ${QT_QTTEST_LIBRARY})


  ############################################

