kde4_header()

add_subdirectory( pics ) 
include_directories( ${CMAKE_SOURCE_DIR}/libkonq  )


########### next target ###############

set(kcm_useraccount_PART_SRCS chfnprocess.cpp main.cpp chfacedlg.cpp )

kde4_automoc(${kcm_useraccount_PART_SRCS})

kde4_add_ui3_files(kcm_useraccount_PART_SRCS main_widget.ui )

kde4_add_kcfg_files(kcm_useraccount_PART_SRCS settings.kcfgc pass.kcfgc )

kde4_add_plugin(kcm_useraccount ${kcm_useraccount_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_useraccount )

target_link_libraries(kcm_useraccount  konq ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDESU_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kcm_useraccount )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kcm_useraccount.desktop  )
install_files( ${KCFG_INSTALL_DIR} FILES kcm_useraccount.kcfg kcm_useraccount_pass.kcfg )

kde4_install_icons( ${ICON_INSTALL_DIR} crystalsvg )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = pics
#kde_module_LTLIBRARIES = kcm_useraccount.la
#kcm_useraccount_la_SOURCES = main_widget.ui chfnprocess.cpp \
#			     main.cpp chfacedlg.cpp settings.kcfgc pass.kcfgc
#kcm_useraccount_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#kcm_useraccount_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
#
#AM_CPPFLAGS = -I$(top_srcdir)/libkonq $(all_includes)
#
#kde_kcfg_DATA = kcm_useraccount.kcfg kcm_useraccount_pass.kcfg
#
#METASOURCES = AUTO
#
#noinst_HEADERS = main.h chfnprocess.h chfacedlg.h
#
#xdg_apps_DATA = kcm_useraccount.desktop 
#
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/useraccount.pot
#KDE_ICON=AUTO
