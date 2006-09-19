


########### next target ###############

set(kcm_css_PART_SRCS template.cpp kcmcss.cpp )

kde4_automoc(${kcm_css_PART_SRCS})

kde4_add_ui3_files(kcm_css_PART_SRCS cssconfig.ui csscustom.ui preview.ui )

kde4_add_plugin(kcm_css ${kcm_css_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_css )

target_link_libraries(kcm_css  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kcm_css  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmcss.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES template.css  DESTINATION  ${DATA_INSTALL_DIR}/kcmcss )

