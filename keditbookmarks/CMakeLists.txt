include_directories( ${CMAKE_SOURCE_DIR}/libkonq  )


########### next target ###############

set(kbookmarkmerger_SRCS kbookmarkmerger.cpp )

kde4_automoc(${kbookmarkmerger_SRCS})

kde4_add_executable(kbookmarkmerger ${kbookmarkmerger_SRCS})

target_link_libraries(kbookmarkmerger  ${KDE4_KIO_LIBS} )

install(TARGETS kbookmarkmerger DESTINATION ${BIN_INSTALL_DIR})


########### next target ###############

set(keditbookmarks_KDEINIT_SRCS 
   main.cpp 
   toplevel.cpp 
   actionsimpl.cpp 
   commands.cpp 
   importers.cpp 
   dcop.cpp 
   bookmarkiterator.cpp 
   testlink.cpp 
   favicons.cpp 
   updater.cpp 
   exporters.cpp 
   bookmarkinfo.cpp 
   kebsearchline.cpp 
   bookmarkmodel.cpp 
   bookmarklistview.cpp 
   treeitem.cpp )

kde4_automoc(${keditbookmarks_KDEINIT_SRCS})



kde4_add_kcfg_files(keditbookmarks_KDEINIT_SRCS settings.kcfgc )

kde4_add_kdeinit_executable( keditbookmarks ${keditbookmarks_KDEINIT_SRCS})

target_link_libraries(kdeinit_keditbookmarks  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS kdeinit_keditbookmarks  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( keditbookmarks kdeinit_keditbookmarks )
install(TARGETS keditbookmarks DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES keditbookmarksui.rc  keditbookmarks-genui.rc  DESTINATION  ${DATA_INSTALL_DIR}/keditbookmarks )

kde4_install_icons( ${ICON_INSTALL_DIR} crystalsvg )




#original Makefile.am contents follow:

#INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = kbookmarkmerger
#lib_LTLIBRARIES =
#kdeinit_LTLIBRARIES = keditbookmarks.la
#
#kbookmarkmerger_SOURCES = kbookmarkmerger.cpp
#kbookmarkmerger_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kbookmarkmerger_LDADD = $(LIB_KIO)
#
#dcop_DCOPIDLNG = true
#keditbookmarks_la_SOURCES = main.cpp toplevel.cpp actionsimpl.cpp commands.cpp importers.cpp dcop.skel dcop.cpp bookmarkiterator.cpp  \
#                            testlink.cpp favicons.cpp updater.cpp exporters.cpp kbookmarknotifier.stub bookmarkinfo.cpp kebsearchline.cpp settings.kcfgc \
#                            bookmarkmodel.cpp bookmarklistview.cpp treeitem.cpp
#
#kbookmarknotifier_DIR = $(includedir)
#keditbookmarks_la_LIBADD  = $(top_builddir)/libkonq/libkonq.la
#keditbookmarks_la_LDFLAGS = $(all_libraries) -module -avoid-version
#
#rcdir = $(kde_datadir)/keditbookmarks
#rc_DATA = keditbookmarksui.rc  keditbookmarks-genui.rc
#
#install-data-local: uninstall.desktop
#	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Internet
#	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Internet/keditbookmarks.desktop
#
#KDE_ICON = AUTO
#
#kde_kcfg_DATA = keditbookmarks.kcfg
#
######## Build rules
#listview.lo: settings.h
#settings.lo: settings.h
#toplevel.lo: settings.h
