#SUBDIRS = init dirtree_module history_module bookmark_module .
SUBDIRS = init . dirtree_module history_module bookmark_module

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes) -I$(srcdir)/..

kde_module_LTLIBRARIES = libkonqsidebar_tree.la

METASOURCES = AUTO

libkonqsidebar_tree_la_SOURCES = konq_sidebartree.cpp konq_sidebartreeitem.cpp konq_sidebartreetoplevelitem.cpp konqsidebar_tree.cpp
libkonqsidebar_tree_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqsidebar_tree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la
