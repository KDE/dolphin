kde4_header()



########### next target ###############

set(kcm_css_PART_SRCS template.cpp kcmcss.cpp )

kde4_automoc(${kcm_css_PART_SRCS})

kde4_add_ui3_files(kcm_css_PART_SRCS cssconfig.ui csscustom.ui preview.ui )

kde4_add_plugin(kcm_css ${kcm_css_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_css )

target_link_libraries(kcm_css  ${KDE4_KDE3SUPPORT_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kcm_css )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kcmcss.desktop )
install_files( ${DATA_INSTALL_DIR}/kcmcss FILES template.css )

kde4_footer()



#original Makefile.am contents follow:

#AM_CPPFLAGS = $(all_includes)
#
#template_DATA = template.css
#templatedir = $(kde_datadir)/kcmcss
#
#kde_module_LTLIBRARIES = kcm_css.la
#kcm_css_la_SOURCES = template.cpp cssconfig.ui csscustom.ui kcmcss.cpp preview.ui
#kcm_css_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_css_la_LIBADD = -lkdeui $(LIB_KIO)
#
#METASOURCES = AUTO
# 
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmcss.pot
# 
#xdg_apps_DATA = kcmcss.desktop
