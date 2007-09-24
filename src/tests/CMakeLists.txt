set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### renamedialogtest ###############


kde4_add_unit_test(renamedialogtest renamedialogtest.cpp)

target_link_libraries(renamedialogtest dolphinprivate ${QT_QTTEST_LIBRARY})


  ############################################

