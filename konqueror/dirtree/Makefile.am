SUBDIRS = init dirtree_module history_module bookmark_module .

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror -I$(top_builddir)/konqueror/dirtree/history_module $(all_includes)

kde_module_LTLIBRARIES = libkonqtree.la

METASOURCES = AUTO

libkonqtree_la_SOURCES = konq_tree.cpp konq_treepart.cpp konq_treeitem.cpp konq_treetoplevelitem.cpp
libkonqtree_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqtree_la_LIBADD = \
	bookmark_module/libkonqbookmarkmodule.la dirtree_module/libkonqdirtreemodule.la history_module/libkonqhistorymodule.la \
	$(top_builddir)/libkonq/libkonq.la # temporary hack (above line)

servicedir = $(kde_servicesdir)
service_DATA = konq_dirtree.desktop
