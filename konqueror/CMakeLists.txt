
project(konqueror)

add_definitions(-D_LARGEFILE64_SOURCE )

set(libkonqueror_intern_KCFG_SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/konq_settingsxt.kcfgc)
kde4_add_kcfg_files(libkonqueror_intern_SRCS ${libkonqueror_intern_KCFG_SRCS})

add_subdirectory( client )
add_subdirectory( iconview )
add_subdirectory( listview )

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( quickprint )
add_subdirectory( remoteencodingplugin )
add_subdirectory( dirpart )
add_subdirectory( settings )

add_subdirectory( kttsplugin )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/workspace/kcontrol/input )
add_subdirectory( tests )


########### next target ###############

set(konqueror_KDEINIT_SRCS  ${libkonqueror_intern_SRCS}
   konq_main.cc
   konq_application.cpp
   konq_guiclients.cc
   konq_run.cc
   konq_view.cc
   konq_viewmgr.cc
   konq_misc.cc
   konq_frame.cc
   konq_framecontainer.cc
   konq_tabs.cc
   konq_actions.cc
   konq_profiledlg.cc
   konq_factory.cc
   konq_combo.cc
   konq_browseriface.cc
   delayedinitializer.cc
   konq_mainwindow.cc
   konq_extensionmanager.cc
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
)

kde4_automoc(${konqueror_KDEINIT_SRCS})


kde4_add_kdeinit_executable( konqueror ${konqueror_KDEINIT_SRCS})

target_link_libraries(kdeinit_konqueror  ${KDE4_KDE3SUPPORT_LIBS} kutils konq )

install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( konqueror kdeinit_konqueror )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############
MESSAGE( STATUS "MIGRATE ALL APPLNK DESKTOP FILE TO XDG DIRECTORY")
install( FILES konqfilemgr.desktop  DESTINATION  ${APPLNK_INSTALL_DIR}/.hidden )
install( FILES kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop  	kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop Home.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.desktop  DESTINATION  ${APPLNK_INSTALL_DIR} )
install( FILES konqueror.rc konq-simplebrowser.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqueror )

install(FILES profile_webbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME webbrowsing)
install(FILES profile_filemanagement.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME filemanagement)
install(FILES profile_midnightcommander.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME midnightcommander)
install(FILES profile_tabbedbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME tabbedbrowsing)
install(FILES profile_kde_devel.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME kde_devel)
install(FILES profile_simplebrowser.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME simplebrowser)

