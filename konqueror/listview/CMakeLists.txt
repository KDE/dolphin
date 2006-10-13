include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############

set(konq_listview_PART_SRCS 
   konq_listview.cc 
   konq_listviewwidget.cc 
   konq_listviewitems.cc 
   konq_treeviewwidget.cc 
   konq_treeviewitem.cc 
   konq_textviewwidget.cc 
   konq_textviewitem.cc 
   konq_infolistviewwidget.cc 
   konq_infolistviewitem.cc )

kde4_automoc(konq_listview ${konq_listview_PART_SRCS})

kde4_add_kcfg_files(konq_listview_PART_SRCS konq_listviewsettings.kcfgc )

kde4_add_plugin(konq_listview ${konq_listview_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_listview )

target_link_libraries(konq_listview  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS konq_listview  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES konq_listview.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konq_treeview.rc konq_detailedlistview.rc konq_textview.rc            konq_infolistview.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqlistview )
install( FILES konq_treeview.desktop konq_detailedlistview.desktop                      konq_textview.desktop konq_infolistview.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


