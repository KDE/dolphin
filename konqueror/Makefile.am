
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

libkonqueror_la_SOURCES = konq_mainview.cc konq_part.cc konq_shell.cc \
			  konq_run.cc konq_childview.cc konq_viewmgr.cc \
			  konq_frame.cc konq_propsview.cc konq_propsmainview.cc \
			  konq_factory.cc konq_events.cc \
			  konq_misc.cc konq_progressproxy.cc \
			  konq_actions.cc konq_profiledlg.cc \
			  browser.moc.cc

libkonqueror_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module

libkonqueror_la_LIBADD = -lkparts ../libkonq/libkonq.la $(LIB_KIO) $(LIB_KDEUI) $(LIB_QT)

browser.moc.cc: $(kde_includes)/browser.h
	$(MOC) $(kde_includes)/browser.h -o $(top_builddir)/konqueror/browser.moc.cc

bin_PROGRAMS = konqueror

konqueror_SOURCES = konq_main.cc KonquerorIface.skel
konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)
konqueror_LDADD = libkonqueror.la

messages:
	$(XGETTEXT) -C  -ktranslate -kaliasLocal -ki18n -x $(includedir)/kde.pot *.h *.cc && mv messages.po ../po/konqueror.pot 
	for subdir in $(SUBDIRS); do \
	  if test ! "x$$subdir" = "x."; then \
	    echo "Making messages in $$subdir"; \
	    (cd $$subdir && $(MAKE) -f Makefile.am messages) || exit 1; \
	  fi ;\
	done

check_PROGRAMS = linktest
#using $(konqueror_SOURCES) below doesn't work (am_edit)
linktest_SOURCES = konq_main.cc KonquerorIface.skel
linktest_LDADD = $(lib_LTLIBRARIES) \
	$(top_builddir)/konqueror/htmlview/libkonqhtmlview.la \
	$(top_builddir)/konqueror/iconview/libkonqiconview.la \
	$(top_builddir)/konqueror/textview/libkonqtextview.la \
	$(top_builddir)/konqueror/treeview/libkonqtreeview.la

linktest_LDFLAGS = $(konqueror_LDFLAGS)

