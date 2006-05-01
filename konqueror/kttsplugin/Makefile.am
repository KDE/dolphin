INCLUDES    = $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkhtmlkttsdplugin.la

libkhtmlkttsdplugin_la_SOURCES = khtmlkttsd.cpp
libkhtmlkttsdplugin_la_LIBADD  = $(LIB_KHTML)
libkhtmlkttsdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/khtml/kpartplugins
plugins_DATA = khtmlkttsd.rc khtmlkttsd.desktop

