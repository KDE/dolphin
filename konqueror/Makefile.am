
INCLUDES = -I$(srcdir)/../libkonq $(all_includes)

SUBDIRS = plugins client

lib_LTLIBRARIES = libkonqhtmlview.la libkonqiconview.la libkonqtxtview.la \
                  libkonqtreeview.la libkonqueror.la

rcdir = $(kde_datadir)/konqueror
rc_DATA = konqueror_part.rc konqueror_shell.rc

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konqueror.desktop konq_htmlview.desktop konq_iconview.desktop \
              konq_txtview.desktop konq_treeview.desktop

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = konq_plugin.desktop

METASOURCES = AUTO

KonquerorIface.kidl: KonquerorIface.h
	( kidl KonquerorIface.h > KonquerorIface.kidl; kidl2cpp --no-stub KonquerorIface.kidl )
KonquerorIface_skel.cpp: KonquerorIface.kidl

libkonqhtmlview_la_SOURCES = konq_htmlview.cc
libkonqhtmlview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqhtmlview_la_LIBADD = libkonqueror.la $(LIB_KHTML)

libkonqiconview_la_SOURCES = konq_iconview.cc
libkonqiconview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqiconview_la_LIBADD = libkonqueror.la 

libkonqtxtview_la_SOURCES = konq_txtview.cc konq_searchdia.cc
libkonqtxtview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqtxtview_la_LIBADD = libkonqueror.la 

libkonqtreeview_la_SOURCES = konq_treeview.cc
libkonqtreeview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqtreeview_la_LIBADD = libkonqueror.la 

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

konqueror_SOURCES = konq_main.cc KonquerorIface_skel.cpp

konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)

konqueror_LDADD = libkonqueror.la

