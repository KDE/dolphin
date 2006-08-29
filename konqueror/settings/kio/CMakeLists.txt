
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

kde4_add_ui_files(kcm_kio_PART_SRCS cache_ui.ui)

kde4_add_ui3_files(kcm_kio_PART_SRCS 
   socksbase.ui 
   kproxydlg_ui.ui 
   kcookiesmanagementdlg_ui.ui 
   kcookiespoliciesdlg_ui.ui 
   policydlg_ui.ui 
   useragentdlg_ui.ui 
   uagentproviderdlg_ui.ui 
   manualproxy_ui.ui 
   envvarproxy_ui.ui)

kde4_add_plugin(kcm_kio ${kcm_kio_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_kio )

target_link_libraries(kcm_kio  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kcm_kio  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES smb.desktop cookies.desktop useragent.desktop cache.desktop lanbrowser.desktop
    netpref.desktop proxy.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES uasprovider.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES socks.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )

