
if(KDE4_BUILD_TESTS)

  include_directories( ${CMAKE_SOURCE_DIR}/libkonq )

  # Tests don't need to go into toplevel/bin, they are fine in the current dir.
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

  ########### konqmimedatatest ###############

  kde4_automoc(konqmimedatatest.cpp)

  kde4_add_executable(konqmimedatatest RUN_UNINSTALLED konqmimedatatest.cpp)

  target_link_libraries(konqmimedatatest konq ${QT_QTTEST_LIBRARY})

  add_test(libkonq-konqmimedatatest konqmimedatatest)

  ########### favicontest ###############

  kde4_automoc(favicontest.cpp)

  kde4_add_executable(favicontest RUN_UNINSTALLED favicontest.cpp)

  target_link_libraries(favicontest konq ${QT_QTTEST_LIBRARY})

  add_test(libkonq-favicontest favicontest)

  ########### historymgrtest ###############

  kde4_automoc(historymgrtest.cpp)

  kde4_add_executable(historymgrtest RUN_UNINSTALLED historymgrtest.cpp)

  target_link_libraries(historymgrtest konq ${QT_QTTEST_LIBRARY})

  add_test(libkonq-historymgrtest historymgrtest)

  ############################################

endif(KDE4_BUILD_TESTS)

