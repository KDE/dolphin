INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../.. $(all_includes)

kde_module_LTLIBRARIES = konq_sidebartree_bookmarks.la

METASOURCES = AUTO

konq_sidebartree_bookmarks_la_SOURCES = bookmark_module.cpp bookmark_item.cpp
konq_sidebartree_bookmarks_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
konq_sidebartree_bookmarks_la_LIBADD = ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la

