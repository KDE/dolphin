set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BUILD_DIR}/.. ${KDE4_INCLUDES} )

# DolphinDetailsView
kde4_add_unit_test(dolphindetailsviewtest TEST dolphindetailsviewtest.cpp testdir.cpp testbase.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphindetailsviewtest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# DolphinSearchBox
if (Nepomuk_FOUND)
  set(dolphinsearchboxtest_SRCS
      dolphinsearchboxtest.cpp
      ../search/dolphinsearchbox.cpp
      ../search/dolphinsearchinformation.cpp
  )
  kde4_add_kcfg_files(dolphinsearchboxtest_SRCS
      ../search/dolphin_searchsettings.kcfgc
  )
  kde4_add_unit_test(dolphinsearchboxtest TEST ${dolphinsearchboxtest_SRCS})
  target_link_libraries(dolphinsearchboxtest ${KDE4_KIO_LIBS} ${SOPRANO_LIBRARIES} ${NEPOMUK_LIBRARIES} ${NEPOMUK_QUERY_LIBRARIES} nepomukutils ${QT_QTTEST_LIBRARY})
endif (Nepomuk_FOUND)


# DolphinTreeView
kde4_add_unit_test(dolphintreeviewtest TEST dolphintreeviewtest.cpp)
target_link_libraries(dolphintreeviewtest dolphinprivate ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})

# DolphinView - columns
kde4_add_unit_test(dolphinviewtest_columns TEST dolphinviewtest_columns.cpp dolphinviewtest_allviewmodes.cpp testdir.cpp testbase.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphinviewtest_columns dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# DolphinView - details
kde4_add_unit_test(dolphinviewtest_details TEST dolphinviewtest_details.cpp dolphinviewtest_allviewmodes.cpp testdir.cpp testbase.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphinviewtest_details dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# DolphinView - icons
kde4_add_unit_test(dolphinviewtest_icons TEST dolphinviewtest_icons.cpp dolphinviewtest_allviewmodes.cpp testdir.cpp testbase.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphinviewtest_icons dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
