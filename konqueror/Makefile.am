
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)

SUBDIRS = . plugins kfmexec client iconview dirtree listview pics

lib_LTLIBRARIES = konqueror.la

rcdir = $(kde_datadir)/konqueror
rc_DATA = konqueror.rc

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konqueror.desktop

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = konq_plugin.desktop
# konq_metaview.desktop

METASOURCES = AUTO

include_HEADERS = KonquerorIface.h

bin_PROGRAMS = konqueror

konqueror_la_SOURCES = konq_main.cc \
	    KonquerorIface.cc KonquerorIface.skel \
	    KonqMainWindowIface.cc KonqMainWindowIface.skel \
	    KonqViewIface.cc KonqViewIface.skel \
	    konq_mainwindow.cc konq_guiclients.cc \
	    konq_run.cc konq_view.cc konq_viewmgr.cc \
	    konq_misc.cc \
	    konq_frame.cc \
	    konq_actions.cc \
	    konq_profiledlg.cc \
	    konq_factory.cc

konqueror_la_LDFLAGS = $(all_libraries) -module -avoid-version
konqueror_la_LIBADD = ../libkonq/libkonq.la

konqueror_SOURCES = dummy.cc
konqueror_LDADD = konqueror.la
konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy.cc:
	echo > dummy.cc

messages: rc.cpp
	$(EXTRACTRC) */*.rc >> rc.cpp
	$(XGETTEXT) -kaliasLocal -o $(podir)/konqueror.pot rc.cpp *.h *.cc *view/*h *view/*cc

data_DATA = konquerorsu.desktop kfmclient.desktop Arrange.desktop
datadir = $(kde_appsdir)/System

data2_DATA = Home.desktop Trash.desktop
data2dir = $(kde_appsdir)

EXTRA_DIST = $(data_DATA) $(data2_DATA)
