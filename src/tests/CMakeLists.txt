if(KDE4_BUILD_TESTS)

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

  # Tests don't need to go into toplevel/bin, they are fine in the current dir.
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

  ########### renamedialogtest ###############

  kde4_automoc(renamedialogtest.cpp)

  kde4_add_executable(renamedialogtest RUN_UNINSTALLED renamedialogtest.cpp)

  target_link_libraries(renamedialogtest dolphinprivate ${QT_QTTEST_LIBRARY})

  add_test(dolphin-renamedialogtest ${EXECUTABLE_OUTPUT_PATH}/renamedialogtest)

  ############################################

endif(KDE4_BUILD_TESTS)

