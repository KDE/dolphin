kde4_header()

add_subdirectory( uasproviders ) 



########### next target ###############

set(kcm_kio_PART_SRCS 
   policydlg.cpp 
   kcookiesmain.cpp 
   kcookiespolicies.cpp 
   main.cpp 
   smbrodlg.cpp 
   useragentdlg.cpp 
   kproxydlgbase.cpp 
   kmanualproxydlg.cpp 
   kenvvarproxydlg.cpp 
   kproxydlg.cpp 
   kcookiesmanagement.cpp 
   fakeuaprovider.cpp 
   uagentproviderdlg.cpp 
   netpref.cpp 
   cache.cpp 
   ksaveioconfig.cpp 
   socks.cpp )

kde4_automoc(${kcm_kio_PART_SRCS})

kde4_add_ui3_files(kcm_kio_PART_SRCS 
   socksbase.ui 
   kproxydlg_ui.ui 
   kcookiesmanagementdlg_ui.ui 
   kcookiespoliciesdlg_ui.ui 
   policydlg_ui.ui 
   useragentdlg_ui.ui 
   uagentproviderdlg_ui.ui 
   manualproxy_ui.ui 
   envvarproxy_ui.ui 
   cache_ui.ui )

kde4_add_plugin(kcm_kio ${kcm_kio_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_kio )

target_link_libraries(kcm_kio  ${KDE4_KDE3SUPPORT_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kcm_kio )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES cookies.desktop useragent.desktop cache.desktop                  lanbrowser.desktop netpref.desktop proxy.desktop )
install_files( ${SERVICETYPES_INSTALL_DIR} FILES uasprovider.desktop )
install_files( ${APPLNK_INSTALL_DIR}/.hidden FILES smb.desktop )
install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES socks.upd )

install(FILES uninstall.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}${APPLNK_INSTALL_DIR}/Settings/WebBrowsing RENAME smb.desktop)


kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = uasproviders
#
#AM_CPPFLAGS = $(all_includes)
#AM_LDFLAGS =  $(all_libraries)
#
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kcm_kio.la
#
#kcm_kio_la_SOURCES = policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp \
#      main.cpp smbrodlg.cpp useragentdlg.cpp kproxydlgbase.cpp \
#      kmanualproxydlg.cpp kenvvarproxydlg.cpp kproxydlg.cpp \
#      kcookiesmanagement.cpp fakeuaprovider.cpp uagentproviderdlg.cpp \
#      netpref.cpp cache.cpp ksaveioconfig.cpp socks.cpp socksbase.ui \
#      kproxydlg_ui.ui kcookiesmanagementdlg_ui.ui kcookiespoliciesdlg_ui.ui \
#      policydlg_ui.ui useragentdlg_ui.ui uagentproviderdlg_ui.ui \
#      manualproxy_ui.ui envvarproxy_ui.ui cache_ui.ui
#
#kcm_kio_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#kcm_kio_la_LIBADD = $(LIB_KIO) $(LIB_KUTILS)
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmkio.pot
#
#update_DATA = socks.upd
#updatedir = $(kde_datadir)/kconf_update
#
#servicetypes_DATA = uasprovider.desktop
#servicetypesdir = $(kde_servicetypesdir)
#
#xdg_apps_DATA = cookies.desktop useragent.desktop cache.desktop \
#                lanbrowser.desktop netpref.desktop proxy.desktop
#
#smb_DATA = smb.desktop
#smbdir = $(kde_appsdir)/.hidden
#
#install-data-local: uninstall.desktop
#	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing
#	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing/smb.desktop
