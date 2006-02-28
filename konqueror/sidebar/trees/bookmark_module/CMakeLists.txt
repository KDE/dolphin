kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(konq_sidebartree_bookmarks_PART_SRCS bookmark_module.cpp bookmark_item.cpp )

kde4_automoc(${konq_sidebartree_bookmarks_PART_SRCS})

kde4_add_plugin(konq_sidebartree_bookmarks ${konq_sidebartree_bookmarks_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartree_bookmarks )

target_link_libraries(konq_sidebartree_bookmarks  ${QT_AND_KDECORE_LIBS} konqsidebarplugin konq_sidebar_tree )

install_targets(${LIB_INSTALL_DIR}/kde4 konq_sidebartree_bookmarks )


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
