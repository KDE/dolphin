INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. $(all_includes) -I$(srcdir)/../..

kde_module_LTLIBRARIES = libkonqsidebartree_dirtree.la

METASOURCES = AUTO
libkonqsidebartree_dirtree_la_SOURCES = dirtree_module.cpp dirtree_item.cpp
libkonqsidebartree_dirtree_la_LDFLAGS = $(all_libraries)  $(KDE_PLUGIN)
libkonqsidebartree_dirtree_la_LIBADD = ../../libkonqsidebarplugin.la  ../libkonqsidebar_tree.la
# $(KDE_PLUGIN) can't do that at the moment

