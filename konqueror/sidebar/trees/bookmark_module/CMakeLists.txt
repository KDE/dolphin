kde4_header()
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

install_targets(${PLUGIN_INSTALL_DIR} konq_sidebartree_bookmarks )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../.. $(all_includes)
#
#kde_module_LTLIBRARIES = konq_sidebartree_bookmarks.la
#
#METASOURCES = AUTO
#
#konq_sidebartree_bookmarks_la_SOURCES = bookmark_module.cpp bookmark_item.cpp
#konq_sidebartree_bookmarks_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#konq_sidebartree_bookmarks_la_LIBADD = ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la
#
