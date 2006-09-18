


########### next target ###############

set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

kde4_automoc(${khtmlkttsdplugin_PART_SRCS})

kde4_add_plugin(khtmlkttsdplugin WITH_PREFIX ${khtmlkttsdplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} khtmlkttsdplugin )

target_link_libraries(khtmlkttsdplugin  ${KDE4_KPARTS_LIBS} ${KDE4_KHTML_LIBS} )

install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
