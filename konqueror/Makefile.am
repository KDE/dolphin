
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)

SUBDIRS = . kfmexec client iconview dirtree listview keditbookmarks kfindpart shellcmdplugin about pics sidebar

lib_LTLIBRARIES = konqueror.la

konqdatadir = $(kde_datadir)/konqueror
konqdata_DATA = konqueror.rc

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konqueror.desktop konqueror_config.desktop

# servicetypedir = $(kde_servicetypesdir)
# servicetype_DATA = konq_metaview.desktop

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
	    konq_factory.cc \
		konq_combo.cc	\
	    konq_browseriface.cc

konqueror_la_LDFLAGS = $(all_libraries) -module -avoid-version
konqueror_la_LIBADD = ../libkonq/libkonq.la

konqueror_SOURCES = dummy.cc
konqueror_LDADD = konqueror.la
konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# Hmm, this experiment of a static konq failed, don't trust it...
# (but feel free to fix it :)

# Note that you need to --enable-debug in kdelibs and kdebase,
# then make clean ; make install in kdelibs and libkonq,
# and make clean ; make konqueror_static in here.
#check_PROGRAMS = konqueror_static
#
#konqueror_static_SOURCES = dummy.cc
#konqueror_static_LDADD = konqueror.la *view/*.la dirtree/*.la $(libdir)/libkhtml.la
#konqueror_static_LDFLAGS = $(all_libraries) -static

dummy.cc:
	echo > dummy.cc

messages: rc.cpp
	$(EXTRACTRC) */*.rc >> rc.cpp
	$(EXTRACTRC) dirtree/history_module/*.ui >> rc.cpp
	$(EXTRACTRC) sidebar/trees/history_module/history_dlg.ui >> rc.cpp
	$(XGETTEXT) -kaliasLocal rc.cpp *.h *.cc *view/*h *view/*cc dirtree/*.cpp dirtree/*h dirtree/*/*.cpp dirtree/*/*.h kedit*/*.cpp about/*.h about/*.cc shellcmdplugin/*.cpp -o `find sidebar -name "*.cpp"` `find sidebar -name "*.h"` -o $(podir)/konqueror.pot

data_DATA = konquerorsu.desktop kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop
datadir = $(kde_appsdir)/System

data2_DATA = Home.desktop
data2dir = $(kde_appsdir)

data3_DATA = konqbrowser.desktop
data3dir = $(kde_appsdir)/Internet

data4_DATA = konqfilemgr.desktop
data4dir = $(kde_appsdir)/.hidden

# Use a hand-made rule for profiles because they are renamed upon installation
profiledir = $(kde_datadir)/konqueror/profiles
install-data-local:
	$(mkinstalldirs) $(profiledir)
	$(INSTALL_DATA) $(srcdir)/profile_webbrowsing.desktop $(profiledir)/webbrowsing
	$(INSTALL_DATA) $(srcdir)/profile_filemanagement.desktop $(profiledir)/filemanagement
	$(INSTALL_DATA) $(srcdir)/profile_midnightcommander.desktop $(profiledir)/midnightcommander
	$(INSTALL_DATA) $(srcdir)/profile_filepreview.desktop $(profiledir)/filepreview
uninstall-local:
	-rm -f $(profiledir)/webbrowsing $(profiledir)/filemanagement $(profiledir)/midnightcommander $(profiledir)/filepreview

