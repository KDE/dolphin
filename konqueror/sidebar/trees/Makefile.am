#SUBDIRS = init dirtree_module history_module bookmark_module .
SUBDIRS = init . dirtree_module history_module bookmark_module

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror -I$(srcdir)/.. $(all_includes)

lib_LTLIBRARIES = libkonq_sidebar.la

METASOURCES = AUTO

libkonq_sidebar_la_SOURCES = konq_sidebartree.cpp konq_sidebartreeitem.cpp konq_sidebartreetoplevelitem.cpp konqsidebar_tree.cpp
libkonq_sidebar_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libkonq_sidebar_la_LIBADD = $(top_builddir)/libkonq/libkonq.la ../libkonqsidebarplugin.la
