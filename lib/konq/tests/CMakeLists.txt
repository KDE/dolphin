kde4_header()

if(KDE4_BUILD_TESTS)

  include_directories( ${CMAKE_SOURCE_DIR}/libkonq )

  # Tests don't need to go into toplevel/bin, they are fine in the current dir.
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

  ########### konqmimedatatest ###############

  kde4_automoc(konqmimedatatest.cpp)

  kde4_add_executable(konqmimedatatest RUN_UNINSTALLED konqmimedatatest.cpp)

  target_link_libraries(konqmimedatatest konq ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

  add_test(libkonq-konqmimedatatest konqmimedatatest)

  ############################################

endif(KDE4_BUILD_TESTS)

kde4_footer()
