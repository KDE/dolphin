include_directories(  ${CMAKE_SOURCE_DIR}/konqueror/sidebar/ ${CMAKE_SOURCE_DIR}/libkonq  )


########### next target ###############

set(konqsidebar_web_PART_SRCS web_module.cpp )

kde4_automoc(konqsidebar_web ${konqsidebar_web_PART_SRCS})

kde4_add_plugin(konqsidebar_web ${konqsidebar_web_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_web )

target_link_libraries(konqsidebar_web  ${KDE4_KDECORE_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KPARTS_LIBS} konqsidebarplugin konq )

install(TARGETS konqsidebar_web  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES websidebar.html  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/websidebar )
install( FILES webmodule_add.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/add )


