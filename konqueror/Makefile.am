
INCLUDES = -I$(srcdir)/../libkonq $(all_includes)

SUBDIRS = . plugins client htmlview iconview textview treeview

lib_LTLIBRARIES = libkonqueror.la

rcdir = $(kde_datadir)/konqueror
rc_DATA = konqueror_part.rc konqueror_shell.rc

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konqueror.desktop 

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = konq_plugin.desktop

METASOURCES = AUTO

include_HEADERS = KonquerorIface.h KonqMainViewIface.h

libkonqueror_la_SOURCES = konq_mainview.cc konq_part.cc konq_shell.cc \
			  konq_run.cc konq_childview.cc konq_viewmgr.cc \
			  konq_frame.cc konq_propsview.cc konq_propsmainview.cc \
			  konq_factory.cc konq_events.cc \
			  konq_misc.cc konq_progressproxy.cc \
			  konq_actions.cc konq_profiledlg.cc \
			  kbrowser.moc.cc

libkonqueror_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module

libkonqueror_la_LIBADD = ../libkonq/libkonq.la -lkparts $(LIB_KDEUI)

kbrowser.moc.cc: $(kde_includes)/kbrowser.h
	$(MOC) $(kde_includes)/kbrowser.h -o $(top_builddir)/konqueror/kbrowser.moc.cc

bin_PROGRAMS = konqueror

konqueror_SOURCES = konq_main.cc KonquerorIface.skel KonqMainViewIface.skel
konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)
konqueror_LDADD = libkonqueror.la $(LIB_KIMGIO)

messages:
	$(XGETTEXT) -C  -ktranslate -kaliasLocal -ki18n -x $(includedir)/kde.pot \
  *.h *.cc *view/*h *view/*cc && mv messages.po ../po/konqueror.pot 

check_PROGRAMS = linktest
#using $(konqueror_SOURCES) below doesn't work (am_edit)
linktest_SOURCES = konq_main.cc KonquerorIface.skel KonqMainViewIface.skel
linktest_LDADD = $(lib_LTLIBRARIES) \
	$(top_builddir)/konqueror/htmlview/libkonqhtmlview.la \
	$(top_builddir)/konqueror/iconview/libkonqiconview.la \
	$(top_builddir)/konqueror/textview/libkonqtextview.la \
	$(top_builddir)/konqueror/treeview/libkonqtreeview.la

linktest_LDFLAGS = $(konqueror_LDFLAGS)

