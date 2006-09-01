include_directories( 
	${CMAKE_SOURCE_DIR}/libkonq 
	${CMAKE_SOURCE_DIR}/konqueror/sidebar/
 ${CMAKE_SOURCE_DIR}/konqueror/sidebar/trees/
	 )


########### next target ###############

set(konq_sidebartree_history_PART_SRCS history_module.cpp history_item.cpp history_settings.cpp ${libkonq_sidebar_tree_SRCS} )

kde4_automoc(${konq_sidebartree_history_PART_SRCS})


kde4_add_plugin(konq_sidebartree_history ${konq_sidebartree_history_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartree_history )

target_link_libraries(konq_sidebartree_history  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq )

install(TARGETS konq_sidebartree_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kcm_history_PART_SRCS kcmhistory.cpp history_settings.cpp )

kde4_automoc(${kcm_history_PART_SRCS})

kde4_add_ui3_files(kcm_history_PART_SRCS history_dlg.ui )


kde4_add_plugin(kcm_history ${kcm_history_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_history )

target_link_libraries(kcm_history  ${KDE4_KDEUI_LIBS} konq )

install(TARGETS kcm_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmhistory.desktop  DESTINATION  ${XDG_APPS_DIR} )

