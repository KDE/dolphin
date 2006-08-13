
project(kfind)

include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(libkfindcommon_SRCS kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp)
set(kfindpart_PART_SRCS kfindpart.cpp ${libkfindcommon_SRCS})

kde4_automoc(${kfindpart_PART_SRCS})

kde4_add_plugin(kfindpart WITH_PREFIX ${kfindpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kfindpart )

target_link_libraries(kfindpart  ${KDE4_KDE3SUPPORT_LIBS} konq )

install(TARGETS kfindpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kfind_SRCS kfwin.cpp kfinddlg.cpp main.cpp ${libkfindcommon_SRCS})

kde4_automoc(${kfind_SRCS})

kde4_add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kfind DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES kfind.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES findpart.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kfindpart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} hicolor )
