include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############

set(konq_part_PART_SRCS 
   konq_part.cc 
   konq_listview.cc 
   konq_iconview.cc 
   konq_selectionmodel.cc 
   konq_itemdelegate.cc 
   konq_iconviewitemdelegate.cc )

kde4_automoc(${konq_part_PART_SRCS})

kde4_add_plugin(konq_part ${konq_part_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_part )

target_link_libraries(konq_part  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS konq_part  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES konq_listview.rc konq_iconview.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqpart )
install( FILES konq_listview.desktop konq_iconview4.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

