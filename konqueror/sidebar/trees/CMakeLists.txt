
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
set(konqsidebar_tree_PART_SRCS ${libkonq_sidebar_tree_SRCS})

kde4_automoc(${konqsidebar_tree_PART_SRCS})

kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_tree )

target_link_libraries(konqsidebar_tree  ${KDE4_KDECORE_LIBS} konq konqsidebarplugin )

install_targets(${PLUGIN_INSTALL_DIR} konqsidebar_tree )


########### install files ###############





#original Makefile.am contents follow:

##SUBDIRS = init dirtree_module history_module bookmark_module .
#SUBDIRS = init . dirtree_module history_module bookmark_module
#
#INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror -I$(srcdir)/.. $(all_includes)
#
#noinst_LTLIBRARIES = libkonq_sidebar_tree.la
#kde_module_LTLIBRARIES = konqsidebar_tree.la
#METASOURCES = AUTO
#
#libkonq_sidebar_tree_la_SOURCES = konq_sidebartree.cpp konq_sidebartreeitem.cpp konq_sidebartreetoplevelitem.cpp
##libkonq_sidebar_tree_la_LDFLAGS = $(all_libraries) -avoid-version -no-undefined
#libkonq_sidebar_tree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la
#
#konqsidebar_tree_la_SOURCES = konqsidebar_tree.cpp
#konqsidebar_tree_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#konqsidebar_tree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la libkonq_sidebar_tree.la
#
