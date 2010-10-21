set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

kde4_add_unit_test(dolphintreeviewtest TEST dolphintreeviewtest.cpp)
target_link_libraries(dolphintreeviewtest dolphinprivate ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY})
