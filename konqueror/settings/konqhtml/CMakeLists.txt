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

kde4_add_ui_files(kcm_konqhtml_PART_SRCS nsconfigwidget.ui advancedTabOptions.ui )

kde4_add_plugin(kcm_konqhtml ${kcm_konqhtml_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_konqhtml )

target_link_libraries(kcm_konqhtml  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KHTML_LIBS})

install(TARGETS kcm_konqhtml  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtml_behavior.desktop khtml_java_js.desktop khtml_fonts.desktop khtml_filter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
