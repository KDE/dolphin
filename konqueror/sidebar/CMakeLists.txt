
add_subdirectory( trees ) 
add_subdirectory( web_module ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/kate/lib  )


########### next target ###############

set(konqsidebarplugin_LIB_SRCS konqsidebarplugin.cpp )

kde4_automoc(${konqsidebarplugin_LIB_SRCS})

kde4_add_library(konqsidebarplugin SHARED ${konqsidebarplugin_LIB_SRCS})

target_link_libraries(konqsidebarplugin  ${KDE4_KPARTS_LIBS} )

set_target_properties(konqsidebarplugin PROPERTIES VERSION 1.2.0 SOVERSION 1 )
install(TARGETS konqsidebarplugin  DESTINATION ${LIB_INSTALL_DIR} )


########### next target ###############

set(konq_sidebar_PART_SRCS konqsidebar.cpp sidebar_widget.cpp )

kde4_automoc(${konq_sidebar_PART_SRCS})

kde4_add_plugin(konq_sidebar ${konq_sidebar_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebar )

target_link_libraries(konq_sidebar  ${KDE4_KPARTS_LIBS} ${KDE4_KIO_LIBS} konq konqsidebarplugin ${KDE4_KUTILS_LIBS} )

install(TARGETS konq_sidebar  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES konq_sidebartng.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES konqsidebartng.rc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES .version  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries/ )
install( FILES konqsidebarplugin.h DESTINATION ${INCLUDE_INSTALL_DIR} )
install( FILES konqsidebartng.upd DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
install( PROGRAMS move_konqsidebartng_entries.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )

