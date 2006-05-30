include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(kcm_konqhtml_PART_SRCS 
   advancedTabDialog.cpp 
   htmlopts.cpp 
   jsopts.cpp 
   javaopts.cpp 
   pluginopts.cpp 
   appearance.cpp 
   khttpoptdlg.cpp 
   policydlg.cpp 
   main.cpp 
   jspolicies.cpp 
   policies.cpp 
   domainlistview.cpp 
   filteropts.cpp )

kde4_automoc(${kcm_konqhtml_PART_SRCS})

kde4_add_ui3_files(kcm_konqhtml_PART_SRCS advancedTabOptions.ui )
kde4_add_ui_files(kcm_konqhtml_PART_SRCS nsconfigwidget.ui )

kde4_add_plugin(kcm_konqhtml ${kcm_konqhtml_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_konqhtml )

target_link_libraries(kcm_konqhtml  ${KDE4_KDE3SUPPORT_LIBS} khtml)

install_targets(${PLUGIN_INSTALL_DIR} kcm_konqhtml )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES khtml_behavior.desktop khtml_java_js.desktop khtml_fonts.desktop khtml_filter.desktop )



#original Makefile.am contents follow:

#AM_CPPFLAGS  = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)
#AM_LDFLAGS =  $(all_libraries)
#
#METASOURCES = AUTO 
#
#kde_module_LTLIBRARIES = kcm_konqhtml.la
#
#kcm_konqhtml_la_SOURCES = advancedTabDialog.cpp htmlopts.cpp jsopts.cpp \
#			     javaopts.cpp pluginopts.cpp appearance.cpp \
#			     khttpoptdlg.cpp policydlg.cpp main.cpp \
#			     jspolicies.cpp nsconfigwidget.ui policies.cpp \
#			     domainlistview.cpp advancedTabOptions.ui \
#			     filteropts.cpp
#
#kcm_konqhtml_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_konqhtml_la_LIBADD = -lkhtml -lkdeui -lkio
#
#xdg_apps_DATA = khtml_behavior.desktop khtml_java_js.desktop khtml_fonts.desktop khtml_filter.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmkonqhtml.pot
#
#install-data-local: uninstall.desktop
#	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing
#	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing/nsplugin.desktop
#	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing/khtml_appearance.desktop
