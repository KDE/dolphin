include(ECMAddAppIcon)

configure_file(config-baloo.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-baloo.h)

configure_file(config-dolphin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-dolphin.h)

configure_file(config-terminal.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-terminal.h)

add_definitions(
    -DTRANSLATION_DOMAIN=\"dolphin\"
)
remove_definitions(
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_SIGNALS_SLOTS_KEYWORDS
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
)

##########################################

set(dolphinvcs_LIB_SRCS
    views/versioncontrol/kversioncontrolplugin.cpp
)

add_library(dolphinvcs ${dolphinvcs_LIB_SRCS})
generate_export_header(dolphinvcs BASE_NAME dolphinvcs)

target_link_libraries(
    dolphinvcs PUBLIC
    Qt5::Widgets
)

set_target_properties(dolphinvcs PROPERTIES
    VERSION ${DOLPHINVCS_VERSION_STRING}
    SOVERSION ${DOLPHINVCS_SOVERSION}
    EXPORT_NAME DolphinVcs
)

ecm_generate_headers(dolphinvcs_LIB_HEADERS
    HEADER_NAMES
    KVersionControlPlugin

    RELATIVE "views/versioncontrol"
    REQUIRED_HEADERS dolphinvcs_LIB_HEADERS
)

install(TARGETS dolphinvcs EXPORT DolphinVcsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES views/versioncontrol/fileviewversioncontrolplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_export.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel)
install(FILES ${dolphinvcs_LIB_HEADERS} DESTINATION "${KDE_INSTALL_INCLUDEDIR}/Dolphin" COMPONENT Devel)

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
    views/versioncontrol/updateitemstatesthread.cpp
    views/versioncontrol/versioncontrolobserver.cpp
    views/viewmodecontroller.cpp
    views/viewproperties.cpp
    views/zoomlevelinfo.cpp
    dolphinremoveaction.cpp
    middleclickactioneventfilter.cpp
    dolphinnewfilemenu.cpp
)

ecm_qt_declare_logging_category(dolphinprivate_LIB_SRCS HEADER dolphindebug.h IDENTIFIER DolphinDebug CATEGORY_NAME org.kde.dolphin)

if(HAVE_BALOO)
    set(dolphinprivate_LIB_SRCS
        ${dolphinprivate_LIB_SRCS}
        views/tooltips/dolphinfilemetadatawidget.cpp
        views/tooltips/tooltipmanager.cpp
        kitemviews/private/kbaloorolesprovider.cpp
    )
endif()

kconfig_add_kcfg_files(dolphinprivate_LIB_SRCS GENERATE_MOC
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

add_library(dolphinprivate ${dolphinprivate_LIB_SRCS})
generate_export_header(dolphinprivate BASE_NAME dolphin)

target_link_libraries(
    dolphinprivate PUBLIC
    dolphinvcs
    Qt5::Concurrent
    Qt5::Gui
    KF5::I18n
    KF5::IconThemes
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::KIOFileWidgets
    KF5::Completion
    KF5::TextWidgets
    KF5::ConfigCore
    KF5::NewStuff
    KF5::Parts
)

if(HAVE_BALOO)
    target_link_libraries(
        dolphinprivate PUBLIC
        KF5::FileMetaData
        KF5::Baloo
        KF5::BalooWidgets
    )
endif()

set_target_properties(dolphinprivate PROPERTIES
    VERSION ${DOLPHINPRIVATE_VERSION_STRING}
    SOVERSION ${DOLPHINPRIVATE_SOVERSION}
)

install(TARGETS dolphinprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphin_export.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel)

##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
   dolphinpart_ext.cpp
   dolphindebug.cpp
)

qt5_add_resources(dolphinpart_SRCS dolphinpart.qrc)

add_library(dolphinpart MODULE ${dolphinpart_SRCS})

target_link_libraries(dolphinpart
    dolphinprivate
)

install(TARGETS dolphinpart DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES dolphinpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

##########################################

set(dolphinstatic_SRCS
    dolphindockwidget.cpp
    dolphinmainwindow.cpp
    dolphinviewcontainer.cpp
    dolphincontextmenu.cpp
    dolphintabbar.cpp
    dolphinplacesmodelsingleton.cpp
    dolphinrecenttabsmenu.cpp
    dolphintabpage.cpp
    dolphintabwidget.cpp
    trash/dolphintrash.cpp
    filterbar/filterbar.cpp
    panels/places/placespanel.cpp
    panels/places/placesitem.cpp
    panels/places/placesitemlistgroupheader.cpp
    panels/places/placesitemlistwidget.cpp
    panels/places/placesitemmodel.cpp
    panels/places/placesitemsignalhandler.cpp
    panels/places/placesview.cpp
    panels/panel.cpp
    panels/folders/foldersitemlistwidget.cpp
    panels/folders/treeviewcontextmenu.cpp
    panels/folders/folderspanel.cpp
    panels/terminal/terminalpanel.cpp
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
    dolphindebug.cpp
    global.cpp
)

if(HAVE_BALOO)
    set(dolphinstatic_SRCS
        ${dolphinstatic_SRCS}
        panels/information/filemetadataconfigurationdialog.cpp
        panels/information/informationpanel.cpp
        panels/information/informationpanelcontent.cpp
        panels/information/pixmapviewer.cpp
        panels/information/phononwidget.cpp
    )
endif()

kconfig_add_kcfg_files(dolphinstatic_SRCS GENERATE_MOC
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

qt5_add_resources(dolphinstatic_SRCS dolphin.qrc)

add_library(dolphinstatic STATIC ${dolphinstatic_SRCS})

target_include_directories(dolphinstatic SYSTEM PRIVATE ${PHONON_INCLUDES})
target_link_libraries(dolphinstatic
    dolphinprivate
    KF5::KCMUtils
    KF5::DBusAddons
    KF5::Notifications
    Phonon::phonon4qt5
)

if (KF5Activities_FOUND)
    target_link_libraries(
        dolphinstatic
        KF5::Activities
    )
endif()

set(dolphin_SRCS
    dbusinterface.cpp
    main.cpp
)

# Sets the icon on Windows and OSX
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*system-file-manager.png")
ecm_add_app_icon(dolphin_SRCS ICONS ${ICONS_SRCS})

kf5_add_kdeinit_executable(dolphin ${dolphin_SRCS})


target_link_libraries(kdeinit_dolphin PRIVATE
    dolphinstatic
    dolphinprivate
    KF5::Crash
)

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeinit_dolphin
    dbusinterface.h
    org.freedesktop.FileManager1.xml
    OPTIONS -a
)

install(TARGETS kdeinit_dolphin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS dolphin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

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

target_link_libraries(kcm_dolphinviewmodes dolphinprivate)
target_link_libraries(kcm_dolphinnavigation dolphinprivate)
target_link_libraries(kcm_dolphinservices dolphinprivate)
target_link_libraries(kcm_dolphingeneral dolphinprivate)

install(TARGETS kcm_dolphinviewmodes DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install(TARGETS kcm_dolphinnavigation DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install(TARGETS kcm_dolphinservices DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install(TARGETS kcm_dolphingeneral DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( PROGRAMS org.kde.dolphin.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install( FILES settings/dolphin_directoryviewpropertysettings.kcfg
               settings/dolphin_generalsettings.kcfg
               settings/dolphin_compactmodesettings.kcfg
               settings/dolphin_iconsmodesettings.kcfg
               settings/dolphin_detailsmodesettings.kcfg
               settings/dolphin_versioncontrolsettings.kcfg
         DESTINATION ${KDE_INSTALL_KCFGDIR} )
install( FILES org.kde.dolphin.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
install( FILES settings/kcm/kcmdolphinviewmodes.desktop DESTINATION
${KDE_INSTALL_KSERVICES5DIR} )
install( FILES settings/kcm/kcmdolphinnavigation.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES settings/kcm/kcmdolphinservices.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES settings/kcm/kcmdolphingeneral.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES settings/services/servicemenu.knsrc DESTINATION ${KDE_INSTALL_CONFDIR} )
install( PROGRAMS settings/services/servicemenuinstallation DESTINATION ${KDE_INSTALL_BINDIR} )
install( PROGRAMS settings/services/servicemenudeinstallation DESTINATION ${KDE_INSTALL_BINDIR} )

if(BUILD_TESTING)
    find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
