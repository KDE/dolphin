include_directories( ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############
set(kded_konqy_preloader_PART_SRCS preloader.cc )

kde4_add_kcfg_files(kded_konqy_preloader kded_konqy_preloader_PART_SRCS ${CMAKE_SOURCE_DIR}/konqueror/konq_settingsxt.kcfgc)

qt4_add_dbus_adaptor(kded_konqy_preloader_PART_SRCS org.kde.konqueror.Preloader.xml preloader.h KonqyPreloader)

kde4_automoc(kded_konqy_preloader ${kded_konqy_preloader_PART_SRCS})


kde4_add_plugin(kded_konqy_preloader ${kded_konqy_preloader_PART_SRCS})

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} kded_konqy_preloader)

target_link_libraries(kded_konqy_preloader  ${KDE4_KDEUI_LIBS})

install(TARGETS kded_konqy_preloader  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES konqy_preload.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES konqy_preloader.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)


