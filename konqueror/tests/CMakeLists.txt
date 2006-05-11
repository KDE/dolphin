kde4_header()

if(KDE4_BUILD_TESTS)

  include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDES} )

  # Tests don't need to go into toplevel/bin, they are fine in the current dir.
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

  ########### konqviewmgrtest ###############

  kde4_automoc(konqviewmgrtest.cpp)

  kde4_add_executable(konqviewmgrtest RUN_UNINSTALLED konqviewmgrtest.cpp)

  target_link_libraries(konqviewmgrtest konq ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

  add_test(libkonq-konqviewmgrtest konqviewmgrtest)

  ############################################

endif(KDE4_BUILD_TESTS)

kde4_footer()
