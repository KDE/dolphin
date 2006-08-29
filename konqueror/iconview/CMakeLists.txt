include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############

set(konq_iconview_PART_SRCS konq_iconview.cc )

kde4_automoc(${konq_iconview_PART_SRCS})

kde4_add_plugin(konq_iconview ${konq_iconview_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_iconview )

target_link_libraries(konq_iconview  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS konq_iconview  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES konq_iconview.rc konq_multicolumnview.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqiconview )
install( FILES konq_iconview.desktop konq_multicolumnview.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

