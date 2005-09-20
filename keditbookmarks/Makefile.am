INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)

METASOURCES = AUTO

bin_PROGRAMS = kbookmarkmerger
lib_LTLIBRARIES =
kdeinit_LTLIBRARIES = keditbookmarks.la

kbookmarkmerger_SOURCES = kbookmarkmerger.cpp
kbookmarkmerger_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kbookmarkmerger_LDADD = $(LIB_KIO)

dcop_DCOPIDLNG = true
keditbookmarks_la_SOURCES = main.cpp toplevel.cpp actionsimpl.cpp commands.cpp importers.cpp dcop.skel dcop.cpp bookmarkiterator.cpp  \
                            testlink.cpp favicons.cpp updater.cpp exporters.cpp kbookmarknotifier.stub bookmarkinfo.cpp kebsearchline.cpp settings.kcfgc \
                            bookmarkmodel.cpp bookmarklistview.cpp treeitem.cpp

kbookmarknotifier_DIR = $(includedir)
keditbookmarks_la_LIBADD  = $(top_builddir)/libkonq/libkonq.la
keditbookmarks_la_LDFLAGS = $(all_libraries) -module -avoid-version

rcdir = $(kde_datadir)/keditbookmarks
rc_DATA = keditbookmarksui.rc  keditbookmarks-genui.rc

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Internet
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Internet/keditbookmarks.desktop

KDE_ICON = AUTO

kde_kcfg_DATA = keditbookmarks.kcfg

####### Build rules
listview.lo: settings.h
settings.lo: settings.h
toplevel.lo: settings.h
