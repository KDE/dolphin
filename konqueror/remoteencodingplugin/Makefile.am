INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
METASOURCES=AUTO

kde_module_LTLIBRARIES = konq_remoteencoding.la
konq_remoteencoding_la_SOURCES = kremoteencodingplugin.cpp
konq_remoteencoding_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
konq_remoteencoding_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

iconviewdir = $(kde_datadir)/konqiconview/kpartplugins
iconview_DATA = kremoteencodingplugin.rc kremoteencodingplugin.desktop

listviewdir = $(kde_datadir)/konqlistview/kpartplugins
listview_DATA = kremoteencodingplugin.rc kremoteencodingplugin.desktop
