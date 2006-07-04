


########### next target ###############

set(kcm_performance_PART_SRCS kcmperformance.cpp konqueror.cpp system.cpp )

kde4_automoc(${kcm_performance_PART_SRCS})

kde4_add_ui3_files(kcm_performance_PART_SRCS konqueror_ui.ui system_ui.ui )

kde4_add_plugin(kcm_performance ${kcm_performance_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_performance )

target_link_libraries(kcm_performance  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kcm_performance  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmperformance.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kcmkonqyperformance.desktop  DESTINATION  ${APPLNK_INSTALL_DIR}/.hidden )




#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kcm_performance.la
#
#kcm_performance_la_SOURCES = kcmperformance.cpp konqueror.cpp konqueror_ui.ui system.cpp system_ui.ui
#kcm_performance_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_performance_la_LIBADD = -lkdeui
#AM_CPPFLAGS= $(all_includes) 
#
#METASOURCES = AUTO
#
#noinst_HEADERS = konqueror.h system.h
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmperformance.pot
#
#xdg_apps_DATA = kcmperformance.desktop
#
#kcmperformance_hidden_data_DATA = kcmkonqyperformance.desktop
#kcmperformance_hidden_datadir = $(kde_appsdir)/.hidden
#
#EXTRA_DIST = $(xdg_apps_DATA)
