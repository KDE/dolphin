INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../..  $(all_includes)

kde_module_LTLIBRARIES = konq_sidebartree_dirtree.la

METASOURCES = AUTO
konq_sidebartree_dirtree_la_SOURCES = dirtree_module.cpp dirtree_item.cpp
konq_sidebartree_dirtree_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
konq_sidebartree_dirtree_la_LIBADD = ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la


