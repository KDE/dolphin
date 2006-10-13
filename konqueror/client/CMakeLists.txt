########### kfmclient ###############

set(kfmclient_SRCS kfmclient.cpp )

qt4_add_dbus_interface( kfmclient_SRCS ../org.kde.Konqueror.Main.xml konq_main_interface )
qt4_add_dbus_interface( kfmclient_SRCS ../org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )

kde4_automoc(kfmclient ${kfmclient_SRCS})

add_definitions(-DQT_NO_CAST_ASCII)

kde4_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})

target_link_libraries(kdeinit_kfmclient  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kdeinit_kfmclient  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kfmclient kdeinit_kfmclient )
install(TARGETS kfmclient DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES kfmclient_3_2.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
install( PROGRAMS kfmclient_3_2_update.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
