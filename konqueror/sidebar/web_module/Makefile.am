INCLUDES = $(all_includes)

kde_module_LTLIBRARIES = konqsidebar_mozilla.la

METASOURCES = AUTO

konqsidebar_mozilla_la_SOURCES = mozilla_module.cpp
konqsidebar_mozilla_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
konqsidebar_mozilla_la_LIBADD = $(LIB_KPARTS) $(LIB_KHTML) -lkonqsidebarplugin

#plugindir=$(kde_datadir)/konqsidebartng/entries
#plugin_DATA=mozillasidebar.desktop

pluginadddir=$(kde_datadir)/konqsidebartng/add
pluginadd_DATA=mozillamodule_add.desktop

htmldir=$(kde_datadir)/konqsidebartng/mozillasidebar
html_DATA=mozillasidebar.html

