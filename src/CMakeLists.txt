add_subdirectory(tests)

macro_optional_find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk" "http://www.kde.org" FALSE "" "For adding desktop-wide tagging support to dolphin")
macro_bool_to_01(Nepomuk_FOUND HAVE_NEPOMUK)
configure_file(config-nepomuk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-nepomuk.h )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

if (Nepomuk_FOUND)
  # Yes, Soprano includes is what we need here
  include_directories( ${SOPRANO_INCLUDE_DIR} ${NEPOMUK_INCLUDE_DIR} )
endif (Nepomuk_FOUND)

########### next target ###############

set(dolphinprivate_LIB_SRCS
    settings/additionalinfodialog.cpp
    settings/applyviewpropsjob.cpp
    settings/dolphinsettings.cpp
    settings/viewpropertiesdialog.cpp
    settings/viewpropsprogressinfo.cpp
    views/additionalinfoaccessor.cpp
    views/dolphincategorydrawer.cpp
    views/dolphindirlister.cpp
    views/dolphinview.cpp
    views/dolphindetailsview.cpp
    views/dolphindetailsviewexpander.cpp
    views/dolphinfileitemdelegate.cpp
    views/dolphiniconsview.cpp
    views/dolphincolumnview.cpp
    views/dolphincolumnviewcontainer.cpp
    views/dolphinmodel.cpp
    views/dolphinnewfilemenuobserver.cpp
    views/dolphinremoteencoding.cpp
    views/dolphinsortfilterproxymodel.cpp
    views/dolphintreeview.cpp
    views/dolphinviewactionhandler.cpp
    views/dolphinviewautoscroller.cpp
    views/dolphinviewcontroller.cpp
    views/draganddrophelper.cpp
    views/folderexpander.cpp
    views/renamedialog.cpp
    views/selectiontoggle.cpp
    views/selectionmanager.cpp
    views/tooltips/filemetadatatooltip.cpp
    views/tooltips/tooltipmanager.cpp
    views/versioncontrol/pendingthreadsmaintainer.cpp
    views/versioncontrol/updateitemstatesthread.cpp
    views/versioncontrol/versioncontrolobserver.cpp
    views/viewextensionsfactory.cpp
    views/viewmodecontroller.cpp
    views/viewproperties.cpp
    views/zoomlevelinfo.cpp
)

kde4_add_kcfg_files(dolphinprivate_LIB_SRCS
    settings/dolphin_columnmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

kde4_add_library(dolphinprivate SHARED ${dolphinprivate_LIB_SRCS})

target_link_libraries(dolphinprivate ${KDE4_KFILE_LIBS} konq ${KDE4_KNEWSTUFF3_LIBS})
if (Nepomuk_FOUND)
  target_link_libraries(dolphinprivate ${NEPOMUK_LIBRARIES} ${NEPOMUK_QUERY_LIBRARIES} nepomukutils ${SOPRANO_LIBRARIES})
endif (Nepomuk_FOUND)


set_target_properties(dolphinprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS dolphinprivate  ${INSTALL_TARGETS_DEFAULT_ARGS})


##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
)

kde4_add_plugin(dolphinpart ${dolphinpart_SRCS})

target_link_libraries(dolphinpart dolphinprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS dolphinpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES dolphinpart.rc DESTINATION ${DATA_INSTALL_DIR}/dolphinpart)
install(FILES dolphinpart.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES views/versioncontrol/fileviewversioncontrolplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

##########################################

set(dolphin_SRCS
    dolphinapplication.cpp
    dolphindockwidget.cpp
    dolphinmainwindow.cpp
    dolphinnewfilemenu.cpp
    dolphinviewcontainer.cpp
    dolphincontextmenu.cpp
    filterbar/filterbar.cpp
    main.cpp
    panels/information/filemetadataconfigurationdialog.cpp
    panels/information/informationpanel.cpp
    panels/information/informationpanelcontent.cpp
    panels/information/pixmapviewer.cpp
    panels/information/phononwidget.cpp
    panels/folders/ktreeview.cpp
    panels/places/placespanel.cpp
    panels/panel.cpp
    panels/folders/treeviewcontextmenu.cpp
    panels/folders/folderspanel.cpp
    panels/folders/paneltreeview.cpp
    search/dolphinsearchbox.cpp
    settings/general/behaviorsettingspage.cpp
    settings/general/contextmenusettingspage.cpp
    settings/general/generalsettingspage.cpp
    settings/general/previewssettingspage.cpp
    settings/general/statusbarsettingspage.cpp
    settings/dolphinsettingsdialog.cpp
    settings/navigation/navigationsettingspage.cpp
    settings/services/servicessettingspage.cpp
    settings/settingspagebase.cpp
    settings/startup/startupsettingspage.cpp
    settings/trash/trashsettingspage.cpp
    settings/viewmodes/columnviewsettingspage.cpp
    settings/viewmodes/detailsviewsettingspage.cpp
    settings/viewmodes/dolphinfontrequester.cpp
    settings/viewmodes/iconsizegroupbox.cpp
    settings/viewmodes/iconsviewsettingspage.cpp
    settings/viewmodes/viewsettingspage.cpp
    settings/viewmodes/viewsettingspagebase.cpp
    statusbar/dolphinstatusbar.cpp
    statusbar/statusbarspaceinfo.cpp
    views/zoomlevelinfo.cpp
)

kde4_add_kcfg_files(dolphin_SRCS
    panels/folders/dolphin_folderspanelsettings.kcfgc
    panels/information/dolphin_informationpanelsettings.kcfgc
    search/dolphin_searchsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

if(Nepomuk_FOUND)
    set(dolphin_SRCS
        ${dolphin_SRCS}
        panels/filter/filterpanel.cpp
    )
endif(Nepomuk_FOUND)

if(NOT WIN32)
   set(dolphin_SRCS ${dolphin_SRCS} panels/terminal/terminalpanel.cpp)
endif(NOT WIN32)

qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.Application.xml
   dolphinapplication.h DolphinApplication)
qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.MainWindow.xml
   dolphinmainwindow.h DolphinMainWindow)

kde4_add_app_icon(dolphin_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

kde4_add_kdeinit_executable(dolphin ${dolphin_SRCS})

target_link_libraries(kdeinit_dolphin
    ${KDE4_KDEPRINT_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KCMUTILS_LIBRARY}
    konq
    dolphinprivate
    knewstuff3
    ${KDE4_PHONON_LIBS}
)

if (Nepomuk_FOUND)
    target_link_libraries(kdeinit_dolphin
        ${NEPOMUK_LIBRARIES}
        ${SOPRANO_LIBRARIES}
        ${NEPOMUK_QUERY_LIBRARIES}
        nepomukutils
    )
endif (Nepomuk_FOUND)

install(TARGETS kdeinit_dolphin ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS dolphin ${INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

set(kcm_dolphinviewmodes_PART_SRCS
    settings/kcm/kcmdolphinviewmodes.cpp
    settings/viewmodes/columnviewsettingspage.cpp
    settings/viewmodes/detailsviewsettingspage.cpp
    settings/viewmodes/dolphinfontrequester.cpp
    settings/viewmodes/iconsizegroupbox.cpp
    settings/viewmodes/iconsviewsettingspage.cpp
    settings/viewmodes/viewsettingspagebase.cpp
    views/zoomlevelinfo.cpp)

set(kcm_dolphinnavigation_PART_SRCS
    settings/kcm/kcmdolphinnavigation.cpp
    settings/navigation/navigationsettingspage.cpp
    settings/settingspagebase.cpp)

set(kcm_dolphinservices_PART_SRCS
    settings/kcm/kcmdolphinservices.cpp
    settings/services/servicessettingspage.cpp
    settings/settingspagebase.cpp)

set(kcm_dolphingeneral_PART_SRCS
    settings/kcm/kcmdolphingeneral.cpp
    settings/general/behaviorsettingspage.cpp
    settings/general/previewssettingspage.cpp
    settings/general/contextmenusettingspage.cpp
    settings/settingspagebase.cpp)

kde4_add_kcfg_files(kcm_dolphinviewmodes_PART_SRCS
    settings/dolphin_columnmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

kde4_add_kcfg_files(kcm_dolphinnavigation_PART_SRCS
    settings/dolphin_generalsettings.kcfgc)

kde4_add_kcfg_files(kcm_dolphinservices_PART_SRCS
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc)

kde4_add_kcfg_files(kcm_dolphingeneral_PART_SRCS
    settings/dolphin_generalsettings.kcfgc)

kde4_add_plugin(kcm_dolphinviewmodes ${kcm_dolphinviewmodes_PART_SRCS})
kde4_add_plugin(kcm_dolphinnavigation ${kcm_dolphinnavigation_PART_SRCS})
kde4_add_plugin(kcm_dolphinservices ${kcm_dolphinservices_PART_SRCS})
kde4_add_plugin(kcm_dolphingeneral ${kcm_dolphingeneral_PART_SRCS})

target_link_libraries(kcm_dolphinviewmodes  ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} dolphinprivate)
target_link_libraries(kcm_dolphinnavigation  ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} dolphinprivate)
target_link_libraries(kcm_dolphinservices  ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KNEWSTUFF3_LIBRARY} dolphinprivate)
target_link_libraries(kcm_dolphingeneral ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_KIO_LIBS} dolphinprivate)
if (Nepomuk_FOUND)
  target_link_libraries(kcm_dolphinviewmodes ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphinnavigation ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphinservices ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphingeneral ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)

install(TARGETS kcm_dolphinviewmodes DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinnavigation DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinservices DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphingeneral DESTINATION ${PLUGIN_INSTALL_DIR} )

#########################################

set(kio_search_PART_SRCS
    search/filenamesearchprotocol.cpp)
kde4_add_plugin(kio_filenamesearch ${kio_search_PART_SRCS})
target_link_libraries(kio_filenamesearch ${KDE4_KIO_LIBS})
install(TARGETS kio_filenamesearch DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( PROGRAMS dolphin.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES settings/dolphin_directoryviewpropertysettings.kcfg
               settings/dolphin_generalsettings.kcfg
               settings/dolphin_columnmodesettings.kcfg
               settings/dolphin_iconsmodesettings.kcfg
               settings/dolphin_detailsmodesettings.kcfg
               settings/dolphin_versioncontrolsettings.kcfg
         DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )
install( FILES search/filenamesearch.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinviewmodes.desktop DESTINATION
${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinnavigation.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinservices.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphingeneral.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/services/servicemenu.knsrc DESTINATION ${CONFIG_INSTALL_DIR} )
install( PROGRAMS settings/services/servicemenuinstallation DESTINATION ${BIN_INSTALL_DIR} )
install( PROGRAMS settings/services/servicemenudeinstallation DESTINATION ${BIN_INSTALL_DIR} )

