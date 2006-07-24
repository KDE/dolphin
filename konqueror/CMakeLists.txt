
project(konqueror)

set(libkonqueror_intern_KCFG_SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/konq_settingsxt.kcfgc)
kde4_add_kcfg_files(libkonqueror_intern_SRCS ${libkonqueror_intern_KCFG_SRCS})

add_subdirectory( client )
add_subdirectory( iconview )
add_subdirectory( listview )

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( quickprint )
add_subdirectory( remoteencodingplugin )
add_subdirectory( dirpart )
add_subdirectory( settings )

add_subdirectory( kttsplugin )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/workspace/kcontrol/input )
add_subdirectory( tests )


########### next target ###############

set(konqueror_KDEINIT_SRCS  ${libkonqueror_intern_SRCS}
   konq_main.cc
   konq_application.cpp
   konq_guiclients.cc
   konq_run.cc
   konq_view.cc
   konq_viewmgr.cc
   konq_misc.cc
   konq_frame.cc
   konq_framecontainer.cc
   konq_tabs.cc
   konq_actions.cc
   konq_profiledlg.cc
   konq_factory.cc
   konq_combo.cc
   konq_browseriface.cc
   delayedinitializer.cc
   konq_mainwindow.cc
   konq_extensionmanager.cc
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
)

kde4_automoc(${konqueror_KDEINIT_SRCS})


kde4_add_kdeinit_executable( konqueror ${konqueror_KDEINIT_SRCS})

target_link_libraries(kdeinit_konqueror  ${KDE4_KDE3SUPPORT_LIBS} kutils konq )

install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( konqueror kdeinit_konqueror )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############
MESSAGE( STATUS "MIGRATE ALL APPLNK DESKTOP FILE TO XDG DIRECTORY")
install( FILES konqfilemgr.desktop  DESTINATION  ${APPLNK_INSTALL_DIR}/.hidden )
install( FILES kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop  	kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop Home.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.desktop  DESTINATION  ${APPLNK_INSTALL_DIR} )
install( FILES konqueror.rc konq-simplebrowser.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqueror )

install(FILES profile_webbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME webbrowsing)
install(FILES profile_filemanagement.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME filemanagement)
install(FILES profile_midnightcommander.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME midnightcommander)
install(FILES profile_tabbedbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME tabbedbrowsing)
install(FILES profile_kde_devel.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME kde_devel)
install(FILES profile_simplebrowser.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME simplebrowser)





#original Makefile.am contents follow:

#AM_CPPFLAGS = -D_LARGEFILE64_SOURCE
#
#INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/workspace/kcontrol/input $(all_includes)
#
#SUBDIRS = . client iconview listview shellcmdplugin about pics sidebar preloader quickprint remoteencodingplugin kttsplugin dirpart settings
#
#bin_PROGRAMS =
#lib_LTLIBRARIES =
#kdeinit_LTLIBRARIES = konqueror.la
#
#noinst_LTLIBRARIES = libkonqueror_intern.la
#konqdatadir = $(kde_datadir)/konqueror
#konqdata_DATA = konqueror.rc konq-simplebrowser.rc
#
#kde_apps_DATA = konqueror.desktop
#
## servicetypedir = $(kde_servicetypesdir)
## servicetype_DATA = konq_metaview.desktop
#
#METASOURCES = AUTO
#
#include_HEADERS = KonquerorIface.h
#
#libkonqueror_intern_la_SOURCES = konq_settingsxt.kcfgc
#konqueror_la_SOURCES = konq_main.cc \
#	    KonquerorIface.cc KonquerorIface.skel \
#	    KonqMainWindowIface.cc KonqMainWindowIface.skel \
#	    KonqViewIface.cc KonqViewIface.skel \
#	    konq_guiclients.cc \
#	    konq_run.cc konq_view.cc konq_viewmgr.cc \
#	    konq_misc.cc \
#	    konq_frame.cc \
#	    konq_tabs.cc \
#	    konq_actions.cc \
#	    konq_profiledlg.cc \
#	    konq_factory.cc \
#	    konq_combo.cc	\
#	    konq_browseriface.cc \
#	    delayedinitializer.cc \
#	    konq_mainwindow.cc  \
#	    konq_extensionmanager.cc
#
#konqueror_la_PCH = AUTO
#konqueror_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#konqueror_la_LIBADD = ../libkonq/libkonq.la libkonqueror_intern.la $(LIBMALLOC) $(LIB_KUTILS)
#
## Hmm, this experiment of a static konq failed, don't trust it...
## (but feel free to fix it :)
#
## Note that you need to --enable-debug in kdelibs and kdebase,
## then make clean ; make install in kdelibs and libkonq,
## and make clean ; make konqueror_static in here.
##check_PROGRAMS = konqueror_static
##
##konqueror_static_SOURCES = dummy.cc
##konqueror_static_LDADD = konqueror.la *view/*.la dirtree/*.la $(libdir)/libkhtml.la
##konqueror_static_LDFLAGS = $(all_libraries) -static
#
#messages-old: rc.cpp
#	$(EXTRACTRC) *.rc */*.rc >> rc.cpp
#	$(EXTRACTRC) sidebar/trees/history_module/history_dlg.ui >> rc.cpp
#	$(XGETTEXT) -kaliasLocal rc.cpp *.h *.cc *view/*h *view/*cc  kedit*/*.cpp about/*.h about/*.cc remoteencodingplugin/*.cpp remoteencodingplugin/*.h shellcmdplugin/*.cpp -o `find sidebar -name "*.cpp"` `find sidebar -name "*.h"` -o $(podir)/konqueror.pot
#
#### TEMPORARY for KDE4, might extract too much
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name \*.rc` >> rc.cpp
#	$(EXTRACTRC) sidebar/trees/history_module/history_dlg.ui >> rc.cpp
#	$(XGETTEXT) -kaliasLocal `find . -name \*.cc -o -name \*.cpp -o -name \*.h` -o $(podir)/konqueror.pot
#
#xdg_apps_DATA = kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop \
#	kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop Home.desktop
#
#konq_data4_DATA = konqfilemgr.desktop
#konq_data4dir = $(kde_appsdir)/.hidden
#
#kde_kcfg_DATA = konqueror.kcfg
#
######## Build rules
#KonquerorIface.lo: konq_settingsxt.h
#konq_actions.lo: konq_settingsxt.h
#konq_guiclients.lo: konq_settingsxt.h
#konq_main.lo: konq_settingsxt.h
#konq_mainwindow.lo: konq_settingsxt.h
#konq_profiledlg.lo: konq_settingsxt.h
#konq_settingsxt.lo: konq_settingsxt.h
#konq_tabs.lo: konq_settingsxt.h
#konq_view.lo: konq_settingsxt.h
#konq_viewmgr.lo: konq_settingsxt.h
#
## Use a hand-made rule for profiles because they are renamed upon installation
#profiledir = $(kde_datadir)/konqueror/profiles
#install-data-local:
#	$(mkinstalldirs) $(DESTDIR)$(profiledir)
#	$(INSTALL_DATA) $(srcdir)/profile_webbrowsing.desktop $(DESTDIR)$(profiledir)/webbrowsing
#	$(INSTALL_DATA) $(srcdir)/profile_filemanagement.desktop $(DESTDIR)$(profiledir)/filemanagement
#	$(INSTALL_DATA) $(srcdir)/profile_midnightcommander.desktop $(DESTDIR)$(profiledir)/midnightcommander
#	$(INSTALL_DATA) $(srcdir)/profile_tabbedbrowsing.desktop $(DESTDIR)$(profiledir)/tabbedbrowsing
#	$(INSTALL_DATA) $(srcdir)/profile_kde_devel.desktop $(DESTDIR)$(profiledir)/kde_devel
#	$(INSTALL_DATA) $(srcdir)/profile_simplebrowser.desktop $(DESTDIR)$(profiledir)/simplebrowser
#
##	$(INSTALL_DATA) $(srcdir)/profile_filepreview.desktop $(DESTDIR)$(profiledir)/filepreview
#
#uninstall-local:
#	-rm -f $(DESTDIR)$(profiledir)/webbrowsing $(DESTDIR)$(profiledir)/filemanagement $(DESTDIR)$(profiledir)/midnightcommander $(DESTDIR)$(profiledir)/filepreview
