
add_subdirectory( plugins ) 



########### next target ###############

set(kcm_kurifilt_PART_SRCS filteropts.cpp main.cpp )

kde4_automoc(${kcm_kurifilt_PART_SRCS})

kde4_add_plugin(kcm_kurifilt ${kcm_kurifilt_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_kurifilt )

target_link_libraries(kcm_kurifilt  ${KDE4_KDEUI_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY} kio)

install_targets(${PLUGIN_INSTALL_DIR} kcm_kurifilt )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES ebrowsing.desktop )




#original Makefile.am contents follow:

#
#SUBDIRS = plugins
#
#AM_CPPFLAGS  = -I$(srcdir)/plugins/ikws $(all_includes)
#
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kcm_kurifilt.la
#kcm_kurifilt_la_SOURCES = filteropts.cpp main.cpp
#
#kcm_kurifilt_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_kurifilt_la_LIBADD = $(LIB_KSYCOCA)
## Don't link this lib to the plugins. They are dlopened and linking to them
## results in crashes when closing kcmshell.
## plugins/ikws/libkuriikwsfilter.la plugins/shorturi/libkshorturifilter.la
#
#messages: rc.cpp
#	$(EXTRACTRC) plugins/*/*.ui >> rc.cpp
#	$(XGETTEXT) *.cpp plugins/ikws/*.cpp plugins/shorturi/*.cpp -o $(podir)/kcmkurifilt.pot
#
#xdg_apps_DATA = ebrowsing.desktop
