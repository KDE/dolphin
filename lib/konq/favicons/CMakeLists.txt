set(kded_favicons_SRCS favicons.cpp favicons_adaptor.cpp)

kde4_automoc(${kded_favicons_SRCS})

kde4_add_plugin(kded_favicons ${kded_favicons_SRCS})

target_link_libraries( kded_favicons ${KDE4_KIO_LIBS} )

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_favicons )

install(TARGETS kded_favicons  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES favicons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

install( FILES favicons.upd move_favicons.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
