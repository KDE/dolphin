set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

kde4_add_unit_test(dolphintreeviewtest TEST dolphintreeviewtest.cpp)
target_link_libraries(dolphintreeviewtest dolphinprivate ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})

kde4_add_unit_test(dolphindetailsviewtest TEST dolphindetailsviewtest.cpp testhelper.cpp ../views/zoomlevelinfo.cpp)
target_link_libraries(dolphindetailsviewtest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
