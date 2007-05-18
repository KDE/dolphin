if(KDE4_BUILD_TESTS)

  include_directories( ${CMAKE_SOURCE_DIR}/apps/dolphin/src )

  # Tests don't need to go into toplevel/bin, they are fine in the current dir.
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

  ########### renamedialogtest ###############

  kde4_automoc(renamedialogtest.cpp)

  kde4_add_executable(renamedialogtest RUN_UNINSTALLED renamedialogtest.cpp)

  target_link_libraries(renamedialogtest dolphin ${QT_QTTEST_LIBRARY})

  ############################################

endif(KDE4_BUILD_TESTS)

