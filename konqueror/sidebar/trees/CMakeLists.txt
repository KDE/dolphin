
set(libkonq_sidebar_tree_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartree.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreeitem.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreetoplevelitem.cpp
	)

add_subdirectory( init ) 
add_subdirectory( dirtree_module ) 
add_subdirectory( history_module ) 
add_subdirectory( bookmark_module ) 
include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror ${CMAKE_SOURCE_DIR}/konqueror/sidebar/ )


########### next target ###############


########### next target ###############
set(konqsidebar_tree_PART_SRCS konqsidebar_tree.cpp ${libkonq_sidebar_tree_SRCS})

kde4_automoc(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})

kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_tree )

target_link_libraries(konqsidebar_tree  ${KDE4_KDECORE_LIBS} konq konqsidebarplugin )

install(TARGETS konqsidebar_tree  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############



