
########### next target ###############

set(kcm_kurifilt_PART_SRCS filteropts.cpp main.cpp )

kde4_automoc(${kcm_kurifilt_PART_SRCS})

kde4_add_plugin(kcm_kurifilt ${kcm_kurifilt_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_kurifilt )

target_link_libraries(kcm_kurifilt  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS kcm_kurifilt  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ebrowsing.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

