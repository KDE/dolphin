set(kded_favicons_SRCS favicons.cpp favicons_adaptor.cpp)

kde4_automoc(${kded_favicons_SRCS})

kde4_add_plugin(kded_favicons ${kded_favicons_SRCS})

target_link_libraries( kded_favicons ${KDE4_KIO_LIBS} )

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_favicons )

install_targets(${PLUGIN_INSTALL_DIR} kded_favicons )

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES favicons.desktop )

install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES favicons.upd move_favicons.sh )
