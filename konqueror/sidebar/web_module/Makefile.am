INCLUDES = -I$(srcdir)/../ -I$(srcdir)/../../../libkonq $(all_includes)

kde_module_LTLIBRARIES = konqsidebar_web.la

METASOURCES = AUTO

konqsidebar_web_la_SOURCES = web_module.cpp
konqsidebar_web_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
konqsidebar_web_la_LIBADD = $(LIB_KPARTS) $(LIB_KHTML) ../libkonqsidebarplugin.la ../../../libkonq/libkonq.la

#plugindir=$(kde_datadir)/konqsidebartng/entries
#plugin_DATA=websidebar.desktop

pluginadddir=$(kde_datadir)/konqsidebartng/add
pluginadd_DATA=webmodule_add.desktop

htmldir=$(kde_datadir)/konqsidebartng/websidebar
html_DATA=websidebar.html

