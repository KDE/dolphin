
configure_file(config-baloo.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-baloo.h)

configure_file(config-dolphin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-dolphin.h)

add_definitions(-DTRANSLATION_DOMAIN=\"dolphin\")
add_definitions(-Wno-deprecated-declarations)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_URL_CAST_FROM_STRING)

if(X11_Xrender_FOUND)
    set(HAVE_XRENDER TRUE)
endif()

configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

########### next target ###############

set(dolphinprivate_LIB_SRCS
    kitemviews/kfileitemlistview.cpp
    kitemviews/kfileitemlistwidget.cpp
    kitemviews/kfileitemmodel.cpp
    kitemviews/kfileitemmodelrolesupdater.cpp
    kitemviews/kitemlistcontainer.cpp
    kitemviews/kitemlistcontroller.cpp
    kitemviews/kitemlistgroupheader.cpp
    kitemviews/kitemlistheader.cpp
    kitemviews/kitemlistselectionmanager.cpp
    kitemviews/kitemliststyleoption.cpp
    kitemviews/kitemlistview.cpp
    kitemviews/kitemlistviewaccessible.cpp
    kitemviews/kitemlistwidget.cpp
    kitemviews/kitemmodelbase.cpp
    kitemviews/kitemset.cpp
    kitemviews/kstandarditem.cpp
    kitemviews/kstandarditemlistgroupheader.cpp
    kitemviews/kstandarditemlistwidget.cpp
    kitemviews/kstandarditemlistview.cpp
    kitemviews/kstandarditemmodel.cpp
    kitemviews/private/kdirectorycontentscounter.cpp
    kitemviews/private/kdirectorycontentscounterworker.cpp
    kitemviews/private/kfileitemclipboard.cpp
    kitemviews/private/kfileitemmodeldirlister.cpp
    kitemviews/private/kfileitemmodelfilter.cpp
    kitemviews/private/kitemlistheaderwidget.cpp
    kitemviews/private/kitemlistkeyboardsearchmanager.cpp
    kitemviews/private/kitemlistroleeditor.cpp
    kitemviews/private/kitemlistrubberband.cpp
    kitemviews/private/kitemlistselectiontoggle.cpp
    kitemviews/private/kitemlistsizehintresolver.cpp
    kitemviews/private/kitemlistsmoothscroller.cpp
    kitemviews/private/kitemlistviewanimation.cpp
    kitemviews/private/kitemlistviewlayouter.cpp
    kitemviews/private/kpixmapmodifier.cpp
    settings/additionalinfodialog.cpp
    settings/applyviewpropsjob.cpp
    settings/viewmodes/viewmodesettings.cpp
    settings/viewpropertiesdialog.cpp
    settings/viewpropsprogressinfo.cpp
    views/dolphinfileitemlistwidget.cpp
    views/dolphinitemlistview.cpp
    views/dolphinnewfilemenuobserver.cpp
    views/dolphinremoteencoding.cpp
    views/dolphinview.cpp
    views/dolphinviewactionhandler.cpp
    views/draganddrophelper.cpp
    views/renamedialog.cpp
    views/tooltips/filemetadatatooltip.cpp
    views/tooltips/tooltipmanager.cpp
    views/versioncontrol/updateitemstatesthread.cpp
    views/versioncontrol/versioncontrolobserver.cpp
    views/viewmodecontroller.cpp
    views/viewproperties.cpp
    views/zoomlevelinfo.cpp
    dolphinremoveaction.cpp
    dolphinnewfilemenu.cpp
)

if(HAVE_BALOO)
    set(dolphinprivate_LIB_SRCS
        ${dolphinprivate_LIB_SRCS}
        kitemviews/private/kbaloorolesprovider.cpp
    )
endif()

kconfig_add_kcfg_files(dolphinprivate_LIB_SRCS
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

add_library(dolphinprivate ${dolphinprivate_LIB_SRCS})

target_link_libraries(
    dolphinprivate PUBLIC
    KF5::KDELibs4Support
    KF5::Konq
    KF5::NewStuff
)

if(HAVE_BALOO)
    target_link_libraries(
        dolphinprivate PUBLIC
        KF5::FileMetaData
        KF5::BalooCore
        KF5::BalooFiles
        KF5::BalooWidgets
    )
endif()

if(X11_Xrender_FOUND)
    target_link_libraries(dolphinprivate ${X11_Xrender_LIB})
endif(X11_Xrender_FOUND)

set_target_properties(dolphinprivate PROPERTIES
    VERSION ${DOLPHINPRIVATE_VERSION_STRING}
    SOVERSION ${DOLPHINPRIVATE_SOVERSION}
)

install(TARGETS dolphinprivate ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
   dolphinpart_ext.cpp
)

add_library(dolphinpart MODULE ${dolphinpart_SRCS})

target_link_libraries(dolphinpart
    dolphinprivate
    KF5::Konq
    KF5::KDELibs4Support
)

install(TARGETS dolphinpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES dolphinpart.rc DESTINATION ${DATA_INSTALL_DIR}/dolphinpart)
install(FILES dolphinpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES views/versioncontrol/fileviewversioncontrolplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

##########################################

set(dolphin_SRCS
    dolphinapplication.cpp
    dolphindockwidget.cpp
    dolphinmainwindow.cpp
    dolphinviewcontainer.cpp
    dolphincontextmenu.cpp
    dolphintabbar.cpp
    dolphinrecenttabsmenu.cpp
    dolphintabpage.cpp
    dolphintabwidget.cpp
    filterbar/filterbar.cpp
    main.cpp
    panels/information/filemetadataconfigurationdialog.cpp
    panels/information/informationpanel.cpp
    panels/information/informationpanelcontent.cpp
    panels/information/pixmapviewer.cpp
    panels/information/phononwidget.cpp
    panels/places/placespanel.cpp
    panels/places/placesitem.cpp
    panels/places/placesitemeditdialog.cpp
    panels/places/placesitemlistgroupheader.cpp
    panels/places/placesitemlistwidget.cpp
    panels/places/placesitemmodel.cpp
    panels/places/placesitemsignalhandler.cpp
    panels/places/placesview.cpp
    panels/panel.cpp
    panels/folders/foldersitemlistwidget.cpp
    panels/folders/treeviewcontextmenu.cpp
    panels/folders/folderspanel.cpp
    search/dolphinfacetswidget.cpp
    search/dolphinsearchbox.cpp
    settings/general/behaviorsettingspage.cpp
    settings/general/configurepreviewplugindialog.cpp
    settings/general/confirmationssettingspage.cpp
    settings/general/generalsettingspage.cpp
    settings/general/previewssettingspage.cpp
    settings/general/statusbarsettingspage.cpp
    settings/dolphinsettingsdialog.cpp
    settings/navigation/navigationsettingspage.cpp
    settings/services/servicessettingspage.cpp
    settings/settingspagebase.cpp
    settings/serviceitemdelegate.cpp
    settings/servicemodel.cpp
    settings/startup/startupsettingspage.cpp
    settings/trash/trashsettingspage.cpp
    settings/viewmodes/dolphinfontrequester.cpp
    settings/viewmodes/viewsettingspage.cpp
    settings/viewmodes/viewmodesettings.cpp
    settings/viewmodes/viewsettingstab.cpp
    statusbar/dolphinstatusbar.cpp
    statusbar/mountpointobserver.cpp
    statusbar/mountpointobservercache.cpp
    statusbar/spaceinfoobserver.cpp
    statusbar/statusbarspaceinfo.cpp
    views/zoomlevelinfo.cpp
)

kconfig_add_kcfg_files(dolphin_SRCS
    panels/folders/dolphin_folderspanelsettings.kcfgc
    panels/information/dolphin_informationpanelsettings.kcfgc
    panels/places/dolphin_placespanelsettings.kcfgc
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    search/dolphin_searchsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

if(NOT WIN32)
   set(dolphin_SRCS ${dolphin_SRCS} panels/terminal/terminalpanel.cpp)
endif()

# TODO Does anything replace kde4_add_app_icon ?
#kde4_add_app_icon(dolphin_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

kf5_add_kdeinit_executable(dolphin ${dolphin_SRCS})

target_include_directories(kdeinit_dolphin PRIVATE ${PHONON_INCLUDES})

target_link_libraries(kdeinit_dolphin
    KF5::Konq
    dolphinprivate
    KF5::NewStuff
    KF5::Parts
    KF5::KCMUtils
    KF5::Solid
    Phonon::phonon4qt5
    KF5::KDELibs4Support
)

if (KF5Activities_FOUND)
    target_link_libraries(
        kdeinit_dolphin
        KF5::Activities
    )
endif()

install(TARGETS kdeinit_dolphin ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS dolphin ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

set(kcm_dolphinviewmodes_PART_SRCS
    settings/kcm/kcmdolphinviewmodes.cpp
    settings/viewmodes/dolphinfontrequester.cpp
    settings/viewmodes/viewmodesettings.cpp
    settings/viewmodes/viewsettingstab.cpp
    views/zoomlevelinfo.cpp)

set(kcm_dolphinnavigation_PART_SRCS
    settings/kcm/kcmdolphinnavigation.cpp
    settings/navigation/navigationsettingspage.cpp
    settings/settingspagebase.cpp)

set(kcm_dolphinservices_PART_SRCS
    settings/kcm/kcmdolphinservices.cpp
    settings/services/servicessettingspage.cpp
    settings/settingspagebase.cpp
    settings/serviceitemdelegate.cpp
    settings/servicemodel.cpp)

set(kcm_dolphingeneral_PART_SRCS
    settings/kcm/kcmdolphingeneral.cpp
    settings/general/behaviorsettingspage.cpp
    settings/general/previewssettingspage.cpp
    settings/general/configurepreviewplugindialog.cpp
    settings/general/confirmationssettingspage.cpp
    settings/settingspagebase.cpp
    settings/serviceitemdelegate.cpp
    settings/servicemodel.cpp)

kconfig_add_kcfg_files(kcm_dolphinviewmodes_PART_SRCS
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

kconfig_add_kcfg_files(kcm_dolphinnavigation_PART_SRCS
    settings/dolphin_generalsettings.kcfgc)

kconfig_add_kcfg_files(kcm_dolphinservices_PART_SRCS
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc)

kconfig_add_kcfg_files(kcm_dolphingeneral_PART_SRCS
    settings/dolphin_generalsettings.kcfgc)

add_library(kcm_dolphinviewmodes MODULE ${kcm_dolphinviewmodes_PART_SRCS})
add_library(kcm_dolphinnavigation MODULE ${kcm_dolphinnavigation_PART_SRCS})
add_library(kcm_dolphinservices MODULE ${kcm_dolphinservices_PART_SRCS})
add_library(kcm_dolphingeneral MODULE ${kcm_dolphingeneral_PART_SRCS})

target_link_libraries(kcm_dolphinviewmodes KF5::KDELibs4Support dolphinprivate)
target_link_libraries(kcm_dolphinnavigation KF5::KDELibs4Support dolphinprivate)
target_link_libraries(kcm_dolphinservices KF5::KDELibs4Support dolphinprivate)
target_link_libraries(kcm_dolphingeneral KF5::KDELibs4Support dolphinprivate)

install(TARGETS kcm_dolphinviewmodes DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinnavigation DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinservices DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphingeneral DESTINATION ${PLUGIN_INSTALL_DIR} )

#########################################

set(kio_search_PART_SRCS
    search/filenamesearchprotocol.cpp)

add_library(kio_filenamesearch MODULE ${kio_search_PART_SRCS})

target_link_libraries(kio_filenamesearch KF5::KDELibs4Support)

install(TARGETS kio_filenamesearch DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( PROGRAMS dolphin.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES settings/dolphin_directoryviewpropertysettings.kcfg
               settings/dolphin_generalsettings.kcfg
               settings/dolphin_compactmodesettings.kcfg
               settings/dolphin_iconsmodesettings.kcfg
               settings/dolphin_detailsmodesettings.kcfg
               settings/dolphin_versioncontrolsettings.kcfg
         DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES dolphinui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/dolphin )
install( FILES dolphin.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/appdata )
install( FILES search/filenamesearch.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinviewmodes.desktop DESTINATION
${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinnavigation.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphinservices.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/kcm/kcmdolphingeneral.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES settings/services/servicemenu.knsrc DESTINATION ${CONFIG_INSTALL_DIR} )
install( PROGRAMS settings/services/servicemenuinstallation DESTINATION ${BIN_INSTALL_DIR} )
install( PROGRAMS settings/services/servicemenudeinstallation DESTINATION ${BIN_INSTALL_DIR} )

add_subdirectory(tests)
