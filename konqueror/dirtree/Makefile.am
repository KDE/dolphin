SUBDIRS = init

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

kde_module_LTLIBRARIES = libkonqdirtree.la libkonqtree.la

METASOURCES = AUTO

libkonqdirtree_la_SOURCES = konq_dirtree.cc
libkonqdirtree_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqdirtree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

libkonqtree_la_SOURCES = konq_tree.cpp konq_treeitem.cpp konq_treepart.cpp
libkonqtree_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqtree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la # add dirtree/blah.la here soon

servicedir = $(kde_servicesdir)
service_DATA = konq_dirtree.desktop
