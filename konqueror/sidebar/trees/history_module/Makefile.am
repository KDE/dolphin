INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. $(all_includes) -I$(srcdir)/../..

kde_module_LTLIBRARIES = libkonqsidebartree_history.la

METASOURCES = AUTO

libkonqsidebartree_history_la_SOURCES = history_module.cpp history_item.cpp \
	history_dlg.ui history_settings.cpp history_settings.skel
libkonqsidebartree_history_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqsidebartree_history_la_LIBADD = ../libkonqsidebar_tree.la ../../libkonqsidebarplugin.la
# $(KDE_PLUGIN)

