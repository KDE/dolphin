include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )

kde4_automoc(konq_shellcmdplugin ${konq_shellcmdplugin_PART_SRCS})

kde4_add_plugin(konq_shellcmdplugin ${konq_shellcmdplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_shellcmdplugin )

target_link_libraries(konq_shellcmdplugin  konq ${KDE4_KDESU_LIBS})

install(TARGETS konq_shellcmdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqlistview/kpartplugins )
install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqiconview/kpartplugins )
