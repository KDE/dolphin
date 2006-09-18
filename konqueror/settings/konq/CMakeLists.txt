include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(kcm_konq_PART_SRCS
   behaviour.cpp
   desktop.cpp
   rootopts.cpp
   fontopts.cpp
   previews.cpp
   browser.cpp
   konqkcmfactory.cpp
   desktopbehavior_impl.cpp )

kde4_automoc(${kcm_konq_PART_SRCS})

kde4_add_ui3_files(kcm_konq_PART_SRCS desktopbehavior.ui )


kde4_add_plugin(kcm_konq ${kcm_konq_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_konq )

target_link_libraries(kcm_konq  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} )

install(TARGETS kcm_konq  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES desktopbehavior.desktop desktop.desktop desktoppath.desktop
    filebehavior.desktop fileappearance.desktop filepreviews.desktop
    filebrowser.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${DATA_INSTALL_DIR}/kcontrol/pics )

