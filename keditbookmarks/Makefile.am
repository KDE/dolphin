INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)

METASOURCES = AUTO

lib_LTLIBRARIES = keditbookmarks.la

keditbookmarks_la_SOURCES = kebbookmarkexporter.cpp core.cpp main.cpp listview.cpp toplevel.cpp actionsimpl.cpp commands.cpp importers.cpp mymanager.cpp dcop.skel dcop.cpp bookmarkiterator.cpp testlink.cpp favicons.cpp search.cpp
kbookmarklistener_DIR = $(top_srcdir)/libkonq

keditbookmarks_la_LIBADD  = $(top_builddir)/libkonq/libkonq.la
keditbookmarks_la_LDFLAGS = $(all_libraries) -module -avoid-version

bin_PROGRAMS = keditbookmarks

keditbookmarks_SOURCES = dummy.cpp
keditbookmarks_LDADD = keditbookmarks.la
keditbookmarks_LDFLAGS = $(all_libraries)

dummy.cpp:
	echo > dummy.cpp

rcdir = $(kde_datadir)/keditbookmarks
rc_DATA = keditbookmarksui.rc  

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Internet
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Internet/keditbookmarks.desktop

KDE_ICON = AUTO

