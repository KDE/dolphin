#SUBDIRS = init dirtree_module history_module bookmark_module .
SUBDIRS = init . dirtree_module history_module bookmark_module

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror -I$(srcdir)/.. $(all_includes)

noinst_LTLIBRARIES = libkonq_sidebar_tree.la
kde_module_LTLIBRARIES = konqsidebar_tree.la
METASOURCES = AUTO

libkonq_sidebar_tree_la_SOURCES = konq_sidebartree.cpp konq_sidebartreeitem.cpp konq_sidebartreetoplevelitem.cpp
#libkonq_sidebar_tree_la_LDFLAGS = $(all_libraries) -avoid-version -no-undefined
libkonq_sidebar_tree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la

konqsidebar_tree_la_SOURCES = konqsidebar_tree.cpp
konqsidebar_tree_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
konqsidebar_tree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la libkonq_sidebar_tree.la

