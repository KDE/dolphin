INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
METASOURCES=AUTO

kde_module_LTLIBRARIES = konq_shellcmdplugin.la
konq_shellcmdplugin_la_SOURCES = kshellcmdexecutor.cpp kshellcmddialog.cpp \
	kshellcmdplugin.cpp
konq_shellcmdplugin_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
konq_shellcmdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

iconviewdir = $(kde_datadir)/konqiconview/kpartplugins
iconview_DATA = kshellcmdplugin.rc

listviewdir = $(kde_datadir)/konqlistview/kpartplugins
listview_DATA = kshellcmdplugin.rc
