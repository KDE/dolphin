INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
METASOURCES=AUTO

kde_module_LTLIBRARIES = libkshellcmdplugin.la
libkshellcmdplugin_la_SOURCES = kshellcmdexecutor.cpp kshellcmddialog.cpp \
	kshellcmdplugin.cpp
libkshellcmdplugin_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
libkshellcmdplugin_la_LDFLAGS = -module $(KDE_PLUGIN)

iconviewdir = $(kde_datadir)/konqiconview/kpartplugins
iconview_DATA = kshellcmdplugin.rc
listviewdir = $(kde_datadir)/konqlistview/kpartplugins
listview_DATA = kshellcmdplugin.rc

