include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(konq_aboutpage_PART_SRCS konq_aboutpage.cc )

kde4_automoc(${konq_aboutpage_PART_SRCS})

kde4_add_plugin(konq_aboutpage ${konq_aboutpage_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_aboutpage )

target_link_libraries(konq_aboutpage  ${KDE4_KHTML_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KDEUI_LIBS}  ${QT_QTGUI_LIBRARY})

install(TARGETS konq_aboutpage  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES box-centre-konqueror.png  top-right-konqueror.png  lightning.png  	launch.html               intro.html        specs.html               tips.html  	plugins.html              plugins_rtl.html  konq.css  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/about )
install( FILES konqaboutpage.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES konq_aboutpage.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

