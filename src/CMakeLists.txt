include(ECMAddAppIcon)

configure_file(config-baloo.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-baloo.h)

configure_file(config-kactivities.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kactivities.h)

configure_file(config-packagekit.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-packagekit.h)

configure_file(config-terminal.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-terminal.h)

configure_file(config-kuserfeedback.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kuserfeedback.h)

add_definitions(
    -DTRANSLATION_DOMAIN=\"dolphin\"
)
remove_definitions(
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
)

##########################################

add_library(dolphinvcs SHARED)

target_sources(dolphinvcs PRIVATE
    views/versioncontrol/kversioncontrolplugin.cpp
)

generate_export_header(dolphinvcs BASE_NAME dolphinvcs)

target_link_libraries(
    dolphinvcs PUBLIC
    Qt5::Widgets
)

set_target_properties(dolphinvcs PROPERTIES
    VERSION ${DOLPHINVCS_VERSION}
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

add_library(dolphinprivate SHARED)

target_sources(dolphinprivate PRIVATE
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
    kitemviews/private/ktwofingerswipe.cpp
    kitemviews/private/ktwofingertap.cpp
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
    views/versioncontrol/updateitemstatesthread.cpp
    views/versioncontrol/versioncontrolobserver.cpp
    views/viewmodecontroller.cpp
    views/viewproperties.cpp
    views/zoomlevelinfo.cpp
    dolphinremoveaction.cpp
    middleclickactioneventfilter.cpp
    dolphinnewfilemenu.cpp
)

ecm_qt_declare_logging_category(dolphinprivate
    HEADER dolphindebug.h
    IDENTIFIER DolphinDebug
    CATEGORY_NAME org.kde.dolphin
    DESCRIPTION "dolphin"
    EXPORT DOLPHIN
)

if(HAVE_BALOO)
    target_sources(dolphinprivate PRIVATE
        views/tooltips/dolphinfilemetadatawidget.cpp
        views/tooltips/tooltipmanager.cpp
        kitemviews/private/kbaloorolesprovider.cpp
    )
endif()

kconfig_add_kcfg_files(dolphinprivate
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_contextmenusettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

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
    KF5::WindowSystem
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
    VERSION ${DOLPHINPRIVATE_VERSION}
    SOVERSION ${DOLPHINPRIVATE_SOVERSION}
)

install(TARGETS dolphinprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphin_export.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel)

##########################################
configure_file(dolphinpart.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/dolphinpart.desktop @ONLY)

add_library(dolphinpart MODULE)

target_sources(dolphinpart PRIVATE
    dolphinpart.cpp
    dolphinpart_ext.cpp
    dolphindebug.cpp

    dolphinpart.qrc
)

kcoreaddons_desktop_to_json(dolphinpart ${CMAKE_CURRENT_BINARY_DIR}/dolphinpart.desktop)

target_link_libraries(dolphinpart
    dolphinprivate
)

install(TARGETS dolphinpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphinpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

##########################################

add_library(dolphinstatic STATIC)

target_sources(dolphinstatic PRIVATE
    dolphinbookmarkhandler.cpp
    dolphindockwidget.cpp
    dolphinmainwindow.cpp
    dolphinviewcontainer.cpp
    dolphincontextmenu.cpp
    dolphinnavigatorswidgetaction.cpp
    dolphintabbar.cpp
    dolphinplacesmodelsingleton.cpp
    dolphinrecenttabsmenu.cpp
    dolphintabpage.cpp
    dolphintabwidget.cpp
    dolphinurlnavigator.cpp
    dolphinurlnavigatorscontroller.cpp
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
    search/dolphinquery.cpp
    search/dolphinsearchbox.cpp
    settings/general/behaviorsettingspage.cpp
    settings/general/configurepreviewplugindialog.cpp
    settings/general/confirmationssettingspage.cpp
    settings/general/generalsettingspage.cpp
    settings/general/previewssettingspage.cpp
    settings/general/statusbarsettingspage.cpp
    settings/dolphinsettingsdialog.cpp
    settings/navigation/navigationsettingspage.cpp
    settings/contextmenu/contextmenusettingspage.cpp
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
    dolphin.qrc
)

if(HAVE_BALOO)
    target_sources(dolphinstatic PRIVATE
        panels/information/informationpanel.cpp
        panels/information/informationpanelcontent.cpp
        panels/information/pixmapviewer.cpp
        panels/information/phononwidget.cpp
    )
endif()

if(HAVE_KUSERFEEDBACK)
    target_sources(dolphinstatic PRIVATE
        userfeedback/dolphinfeedbackprovider.cpp
        userfeedback/settingsdatasource.cpp
        userfeedback/placesdatasource.cpp
        settings/userfeedback/userfeedbacksettingspage.cpp
    )
endif()

kconfig_add_kcfg_files(dolphinstatic
    panels/folders/dolphin_folderspanelsettings.kcfgc
    panels/information/dolphin_informationpanelsettings.kcfgc
    panels/places/dolphin_placespanelsettings.kcfgc
    settings/dolphin_compactmodesettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_contextmenusettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    search/dolphin_searchsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)
kconfig_add_kcfg_files(dolphinstatic GENERATE_MOC
    settings/dolphin_generalsettings.kcfgc
)

qt5_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dolphinmainwindow.h org.kde.DolphinMainWindow.xml)
qt5_add_dbus_adaptor(dolphin_dbus_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.DolphinMainWindow.xml dolphinmainwindow.h DolphinMainWindow)
qt5_add_dbus_interface(dolphin_dbus_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.DolphinMainWindow.xml dolphinmainwindowinterface)
qt5_add_dbus_interface(dolphin_dbus_SRCS panels/terminal/org.kde.KIOFuse.VFS.xml kiofuse_interface)

target_sources(dolphinstatic PRIVATE
    ${dolphin_dbus_SRCS}
)

target_include_directories(dolphinstatic SYSTEM PRIVATE ${PHONON_INCLUDES})
target_link_libraries(dolphinstatic
    dolphinprivate
    KF5::CoreAddons
    KF5::KCMUtils
    KF5::DBusAddons
    KF5::Notifications
    Phonon::phonon4qt5
)

if (HAVE_KACTIVITIES)
    target_link_libraries(
        dolphinstatic
        KF5::Activities
    )
endif()

if (HAVE_KUSERFEEDBACK)
    target_link_libraries(
        dolphinstatic
        KUserFeedbackCore
        KUserFeedbackWidgets
    )
endif()

add_executable(dolphin)

target_sources(dolphin PRIVATE
    dbusinterface.cpp
    main.cpp
)

if(FLATPAK)
    target_compile_definitions(dolphin PRIVATE FLATPAK)
endif()

# Sets the icon on Windows and OSX
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*system-file-manager.png")
ecm_add_app_icon(dolphin_APPICON_SRCS ICONS ${ICONS_SRCS})
# ecm_add_app_icon 5.83 will take target as arg, use that once it is min req.
target_sources(dolphin PRIVATE
    ${dolphin_APPICON_SRCS}
)

target_link_libraries(dolphin
    PRIVATE
    dolphinprivate
    dolphinstatic
    KF5::Crash
)

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    dolphin
    dbusinterface.h
    org.freedesktop.FileManager1.xml
    OPTIONS -a
)

install(TARGETS dolphin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

if(NOT WIN32)
    # The settings are still accessible from the hamburger menu
    add_library(kcm_dolphinviewmodes)
    kcoreaddons_desktop_to_json(kcm_dolphinviewmodes settings/kcm/kcmdolphinviewmodes.desktop)
    add_library(kcm_dolphinnavigation)
    kcoreaddons_desktop_to_json(kcm_dolphinnavigation settings/kcm/kcmdolphinnavigation.desktop)
    add_library(kcm_dolphingeneral)
    kcoreaddons_desktop_to_json(kcm_dolphingeneral settings/kcm/kcmdolphingeneral.desktop)

    target_sources(kcm_dolphinviewmodes PRIVATE
        settings/kcm/kcmdolphinviewmodes.cpp
        settings/viewmodes/dolphinfontrequester.cpp
        settings/viewmodes/viewmodesettings.cpp
        settings/viewmodes/viewsettingstab.cpp
        views/zoomlevelinfo.cpp)

    target_sources(kcm_dolphinnavigation PRIVATE
        settings/kcm/kcmdolphinnavigation.cpp
        settings/navigation/navigationsettingspage.cpp
        settings/settingspagebase.cpp)

    target_sources(kcm_dolphingeneral PRIVATE
        settings/kcm/kcmdolphingeneral.cpp
        settings/general/behaviorsettingspage.cpp
        settings/general/previewssettingspage.cpp
        settings/general/configurepreviewplugindialog.cpp
        settings/general/confirmationssettingspage.cpp
        settings/settingspagebase.cpp
        settings/serviceitemdelegate.cpp
        settings/servicemodel.cpp)

    kconfig_add_kcfg_files(kcm_dolphinviewmodes
        settings/dolphin_compactmodesettings.kcfgc
        settings/dolphin_directoryviewpropertysettings.kcfgc
        settings/dolphin_detailsmodesettings.kcfgc
        settings/dolphin_iconsmodesettings.kcfgc
        settings/dolphin_generalsettings.kcfgc
        settings/dolphin_versioncontrolsettings.kcfgc
    )

    kconfig_add_kcfg_files(kcm_dolphinnavigation
        settings/dolphin_generalsettings.kcfgc)

    kconfig_add_kcfg_files(kcm_dolphingeneral
        settings/dolphin_generalsettings.kcfgc)

    target_link_libraries(kcm_dolphinviewmodes dolphinprivate)
    target_link_libraries(kcm_dolphinnavigation dolphinprivate)
    target_link_libraries(kcm_dolphingeneral dolphinprivate)

    install( FILES org.kde.dolphin.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
    install( FILES settings/kcm/kcmdolphinviewmodes.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
    install( FILES settings/kcm/kcmdolphinnavigation.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
    install( FILES settings/kcm/kcmdolphingeneral.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

    install(TARGETS kcm_dolphinviewmodes DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphin/kcms )
    install(TARGETS kcm_dolphinnavigation DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphin/kcms )
    install(TARGETS kcm_dolphingeneral DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphin/kcms )
endif()

if(NOT WIN32)
    add_subdirectory(settings/contextmenu/servicemenuinstaller)
    install( FILES settings/contextmenu/servicemenu.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR} )
endif()

########### install files ###############

install( PROGRAMS org.kde.dolphin.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

install( DIRECTORY DESTINATION "${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel" )

install(
    CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"${KDE_INSTALL_FULL_APPDIR}/org.kde.dolphin.desktop\" \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel/org.kde.dolphin.desktop\")"
)

install( FILES settings/dolphin_directoryviewpropertysettings.kcfg
               settings/dolphin_generalsettings.kcfg
               settings/dolphin_contextmenusettings.kcfg
               settings/dolphin_compactmodesettings.kcfg
               settings/dolphin_iconsmodesettings.kcfg
               settings/dolphin_detailsmodesettings.kcfg
               settings/dolphin_versioncontrolsettings.kcfg
         DESTINATION ${KDE_INSTALL_KCFGDIR} )

if(BUILD_TESTING)
    find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
