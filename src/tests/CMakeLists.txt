set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

kde4_add_unit_test(dolphintreeviewtest TEST dolphintreeviewtest.cpp)
target_link_libraries(dolphintreeviewtest dolphinprivate ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})

kde4_add_unit_test(dolphindetailsviewtest TEST dolphindetailsviewtest.cpp testbase.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphindetailsviewtest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

kde4_add_unit_test(dolphinviewtest_icons TEST dolphinviewtest_icons.cpp dolphinviewtest_allviewmodes.cpp testbase.cpp)
target_link_libraries(dolphinviewtest_icons dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

kde4_add_unit_test(dolphinviewtest_details TEST dolphinviewtest_details.cpp dolphinviewtest_allviewmodes.cpp testbase.cpp)
target_link_libraries(dolphinviewtest_details dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

kde4_add_unit_test(dolphinviewtest_columns TEST dolphinviewtest_columns.cpp dolphinviewtest_allviewmodes.cpp testbase.cpp)
target_link_libraries(dolphinviewtest_columns dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
