


########### next target ###############

set(konq_sidebartest_PART_SRCS konqsidebartest.cpp )

kde4_automoc(${konq_sidebartest_PART_SRCS})

kde4_add_plugin(konq_sidebartest ${konq_sidebartest_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartest )

target_link_libraries(konq_sidebartest ${KDE4_KPARTS_LIBS} ${KDE4_KIO_LIBS} konqsidebarplugin )

install(TARGETS konq_sidebartest  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES test.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries )


