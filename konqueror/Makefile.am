
INCLUDES = -I$(srcdir)/../libkonq $(all_includes)

SUBDIRS = plugins

lib_LTLIBRARIES = libkonqueror.la

rcdir = $(kde_datadir)/konqueror
rc_DATA = konqueror_part.rc konqueror_shell.rc

libkonqueror_la_SOURCES = konq_mainview.cc konq_part.cc konq_shell.cc \
			  konq_run.cc konq_childview.cc konq_viewmgr.cc \
			  konq_frame.cc konq_propsview.cc konq_propsmainview.cc \
			  konq_factory.cc konq_events.cc konq_iconview.cc \
			  konq_searchdia.cc konq_txtview.cc \
			  konq_treeview.cc konq_misc.cc konq_progressproxy.cc \
			  konq_htmlview.cc konq_actions.cc browser.moc.cc

libkonqueror_la_METASOURCES = AUTO

libkonqueror_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -avoid-version

libkonqueror_la_LIBADD = -lkparts ../libkonq/libkonq.la $(LIB_KIO) $(LIB_KDEUI) $(LIB_QT) $(LIB_KHTML)

browser.moc.cc: $(kde_includes)/browser.h
	$(MOC) $(kde_includes)/browser.h -o $(top_builddir)/konqueror/browser.moc.cc

bin_PROGRAMS = konqueror

konqueror_SOURCES = konq_main.cc

konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)

konqueror_LDADD = libkonqueror.la -lkded 

