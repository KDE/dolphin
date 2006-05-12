include_directories( 
	${CMAKE_SOURCE_DIR}/konqueror/sidebar/ 
	${CMAKE_SOURCE_DIR}/konqueror/sidebar/trees/
	${CMAKE_SOURCE_DIR}/libkonq 
	 )


########### next target ###############

set(konq_sidebartree_dirtree_PART_SRCS dirtree_module.cpp dirtree_item.cpp ${libkonq_sidebar_tree_SRCS} )

kde4_automoc(${konq_sidebartree_dirtree_PART_SRCS})

kde4_add_plugin(konq_sidebartree_dirtree ${konq_sidebartree_dirtree_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartree_dirtree )

target_link_libraries(konq_sidebartree_dirtree  ${KDE4_KDE3SUPPORT_LIBS} konq konqsidebarplugin  )

install_targets(${PLUGIN_INSTALL_DIR} konq_sidebartree_dirtree )


########### install files ###############





#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../..  $(all_includes)
#
#kde_module_LTLIBRARIES = konq_sidebartree_dirtree.la
#
#METASOURCES = AUTO
#konq_sidebartree_dirtree_la_SOURCES = dirtree_module.cpp dirtree_item.cpp
#konq_sidebartree_dirtree_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#konq_sidebartree_dirtree_la_LIBADD = ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la
#
#
