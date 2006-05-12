kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/konqueror ${CMAKE_SOURCE_DIR}/workspace/kdesktop/ )


########### next target ###############

set(kfmclient_KDEINIT_SRCS kfmclient.cc )

kde4_automoc(${kfmclient_KDEINIT_SRCS})

kde4_add_dcop_stubs(kfmclient_KDEINIT_SRCS ${CMAKE_SOURCE_DIR}/konqueror/KonquerorIface.h ${CMAKE_SOURCE_DIR}/workspace/kdesktop/KDesktopIface.h )

kde4_add_kdeinit_executable( kfmclient ${kfmclient_KDEINIT_SRCS})

target_link_libraries(kdeinit_kfmclient  ${KDE4_KDE3SUPPORT_LIBS} )

install_targets(${LIB_INSTALL_DIR} kdeinit_kfmclient )

target_link_libraries( kfmclient kdeinit_kfmclient )
install(TARGETS kfmclient DESTINATION bin)

########### install files ###############

install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES kfmclient_3_2.upd )
install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES kfmclient_3_2_update.sh )


kde4_footer()



#original Makefile.am contents follow:

#AM_CPPFLAGS = -DQT_NO_CAST_ASCII
#
#bin_PROGRAMS= 
#lib_LTLIBRARIES = 
#kdeinit_LTLIBRARIES = kfmclient.la
#
#INCLUDES= -I.. $(all_includes)
#kfmclient_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kfmclient_la_LIBADD =  $(LIB_KIO)
#kfmclient_la_SOURCES = kfmclient.cc KonquerorIface.stub KDesktopIface.stub
#
#noinst_HEADERS = kfmclient.h
#METASOURCES = AUTO
#
#KonquerorIface_DIR =  $(srcdir)/..
#KDesktopIface_DIR = $(top_srcdir)/workspace/kdesktop
#
#messages:
#	$(XGETTEXT) -kaliasLocal *.h *.cc -o $(podir)/kfmclient.pot
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = kfmclient_3_2.upd
#update_SCRIPTS = kfmclient_3_2_update.sh
