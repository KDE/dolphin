INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. $(all_includes) -I$(srcdir)/../..

kde_module_LTLIBRARIES = libkonqsidebartree_bookmarks.la

METASOURCES = AUTO

libkonqsidebartree_bookmarks_la_SOURCES = bookmark_module.cpp bookmark_item.cpp
libkonqsidebartree_bookmarks_la_LDFLAGS = $(all_libraries)
libkonqsidebartree_bookmarks_la_LIBADD = ../libkonqsidebar_tree.la ../../libkonqsidebarplugin.la
# $(KDE_PLUGIN) can't do that at the moment

