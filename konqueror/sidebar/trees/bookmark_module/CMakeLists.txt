include_directories( 
	${CMAKE_SOURCE_DIR}/libkonq 
 ${CMAKE_SOURCE_DIR}/konqueror/sidebar/
 ${CMAKE_SOURCE_DIR}/konqueror/sidebar/trees/
	 )


########### next target ###############

set(konq_sidebartree_bookmarks_PART_SRCS bookmark_module.cpp bookmark_item.cpp ${libkonq_sidebar_tree_SRCS} )

kde4_automoc(${konq_sidebartree_bookmarks_PART_SRCS})

kde4_add_plugin(konq_sidebartree_bookmarks ${konq_sidebartree_bookmarks_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartree_bookmarks )

target_link_libraries(konq_sidebartree_bookmarks  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq)

install(TARGETS konq_sidebartree_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )

