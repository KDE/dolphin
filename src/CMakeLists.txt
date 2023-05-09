include(ECMAddAppIcon)

configure_file(config-dolphin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-dolphin.h)

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
    Qt${QT_MAJOR_VERSION}::Widgets
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
target_include_directories(dolphinvcs INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>")

install(TARGETS dolphinvcs EXPORT DolphinVcsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_export.h DESTINATION ${KDE_INSTALL_INCLUDEDIR}/Dolphin COMPONENT Devel)
install(FILES ${dolphinvcs_LIB_HEADERS} DESTINATION "${KDE_INSTALL_INCLUDEDIR}/Dolphin" COMPONENT Devel)

########### next target ###############

add_library(dolphinprivate SHARED)

target_sources(dolphinprivate PRIVATE
    kitemviews/kfileitemlistview.cpp
    kitemviews/kfileitemlistwidget.cpp
    kitemviews/kfileitemlisttostring.cpp
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
    kitemviews/kstandarditemlistgroupheader.cpp
    kitemviews/kstandarditemlistwidget.cpp
    kitemviews/kstandarditemlistview.cpp
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
    settings/applyviewpropsjob.cpp
    settings/viewmodes/viewmodesettings.cpp
    settings/viewpropertiesdialog.cpp
    settings/viewpropsprogressinfo.cpp
    selectionmode/actiontexthelper.cpp
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

    kitemviews/kfileitemlistview.h
    kitemviews/kfileitemlistwidget.h
    kitemviews/kfileitemlisttostring.h
    kitemviews/kfileitemmodel.h
    kitemviews/kfileitemmodelrolesupdater.h
    kitemviews/kitemlistcontainer.h
    kitemviews/kitemlistcontroller.h
    kitemviews/kitemlistgroupheader.h
    kitemviews/kitemlistheader.h
    kitemviews/kitemlistselectionmanager.h
    kitemviews/kitemliststyleoption.h
    kitemviews/kitemlistview.h
    kitemviews/kitemlistviewaccessible.h
    kitemviews/kitemlistwidget.h
    kitemviews/kitemmodelbase.h
    kitemviews/kitemset.h
    kitemviews/kstandarditemlistgroupheader.h
    kitemviews/kstandarditemlistwidget.h
    kitemviews/kstandarditemlistview.h
    kitemviews/private/kdirectorycontentscounter.h
    kitemviews/private/kdirectorycontentscounterworker.h
    kitemviews/private/kfileitemclipboard.h
    kitemviews/private/kfileitemmodelfilter.h
    kitemviews/private/kitemlistheaderwidget.h
    kitemviews/private/kitemlistkeyboardsearchmanager.h
    kitemviews/private/kitemlistroleeditor.h
    kitemviews/private/kitemlistrubberband.h
    kitemviews/private/kitemlistselectiontoggle.h
    kitemviews/private/kitemlistsizehintresolver.h
    kitemviews/private/kitemlistsmoothscroller.h
    kitemviews/private/kitemlistviewanimation.h
    kitemviews/private/kitemlistviewlayouter.h
    kitemviews/private/kpixmapmodifier.h
    settings/applyviewpropsjob.h
    settings/viewmodes/viewmodesettings.h
    settings/viewpropertiesdialog.h
    settings/viewpropsprogressinfo.h
    selectionmode/actiontexthelper.h
    views/dolphinfileitemlistwidget.h
    views/dolphinitemlistview.h
    views/dolphinnewfilemenuobserver.h
    views/dolphinremoteencoding.h
    views/dolphinview.h
    views/dolphinviewactionhandler.h
    views/draganddrophelper.h
    views/versioncontrol/updateitemstatesthread.h
    views/versioncontrol/versioncontrolobserver.h
    views/viewmodecontroller.h
    views/viewproperties.h
    views/zoomlevelinfo.h
    dolphinremoveaction.h
    middleclickactioneventfilter.h
    dolphinnewfilemenu.h
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
        views/tooltips/dolphinfilemetadatawidget.h
        views/tooltips/tooltipmanager.h
        kitemviews/private/kbaloorolesprovider.h
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
    Qt${QT_MAJOR_VERSION}::Concurrent
    Qt${QT_MAJOR_VERSION}::Gui
    KF6::I18n
    KF6::IconThemes
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    KF6::Completion
    KF6::TextWidgets
    KF6::ConfigCore
    KF6::NewStuffWidgets    # KNSWidgets::Button
    KF6::Parts
    KF6::WindowSystem
    KF6::WidgetsAddons
    KF6::Codecs
    KF6::KCMUtils

    KF6::MoreTools
)

if(HAVE_BALOO)
    target_link_libraries(
        dolphinprivate PUBLIC
        KF6::FileMetaData
        KF6::Baloo
        KF6::BalooWidgets
    )
endif()

set_target_properties(dolphinprivate PROPERTIES
    VERSION ${DOLPHINPRIVATE_VERSION}
    SOVERSION ${DOLPHINPRIVATE_SOVERSION}
)

install(TARGETS dolphinprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dolphin_export.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel)

##########################################
configure_file(dolphinpart.json.in ${CMAKE_CURRENT_BINARY_DIR}/dolphinpart.json @ONLY)

add_library(dolphinpart MODULE)

target_sources(dolphinpart PRIVATE
    dolphinpart.cpp
    dolphinpart_ext.cpp
    dolphindebug.cpp
    dolphinpart.h
    dolphinpart_ext.h
    dolphindebug.h

    dolphinpart.qrc
)

target_link_libraries(dolphinpart
    dolphinprivate
)

install(TARGETS dolphinpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/parts)
install(FILES dolphinpartactions.desktop DESTINATION "${KDE_INSTALL_DATADIR}/dolphin/")

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
    kitemviews/kfileitemlisttostring.cpp
    panels/places/placespanel.cpp
    panels/panel.cpp
    panels/folders/foldersitemlistwidget.cpp
    panels/folders/treeviewcontextmenu.cpp
    panels/folders/folderspanel.cpp
    panels/terminal/terminalpanel.cpp
    search/dolphinfacetswidget.cpp
    search/dolphinquery.cpp
    search/dolphinsearchbox.cpp
    selectionmode/actiontexthelper.cpp
    selectionmode/actionwithwidget.cpp
    selectionmode/backgroundcolorhelper.cpp
    selectionmode/bottombar.cpp
    selectionmode/bottombarcontentscontainer.cpp
    selectionmode/topbar.cpp
    settings/general/behaviorsettingspage.cpp
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

    dolphinbookmarkhandler.h
    dolphindockwidget.h
    dolphinmainwindow.h
    dolphinviewcontainer.h
    dolphincontextmenu.h
    dolphinnavigatorswidgetaction.h
    dolphintabbar.h
    dolphinplacesmodelsingleton.h
    dolphinrecenttabsmenu.h
    dolphintabpage.h
    dolphintabwidget.h
    dolphinurlnavigator.h
    dolphinurlnavigatorscontroller.h
    trash/dolphintrash.h
    filterbar/filterbar.h
    kitemviews/kfileitemlisttostring.h
    panels/places/placespanel.h
    panels/panel.h
    panels/folders/foldersitemlistwidget.h
    panels/folders/treeviewcontextmenu.h
    panels/folders/folderspanel.h
    panels/terminal/terminalpanel.h
    search/dolphinfacetswidget.h
    search/dolphinquery.h
    search/dolphinsearchbox.h
    selectionmode/actiontexthelper.h
    selectionmode/actionwithwidget.h
    selectionmode/backgroundcolorhelper.h
    selectionmode/bottombar.h
    selectionmode/bottombarcontentscontainer.h
    selectionmode/topbar.h
    settings/general/behaviorsettingspage.h
    settings/general/confirmationssettingspage.h
    settings/general/generalsettingspage.h
    settings/general/previewssettingspage.h
    settings/general/statusbarsettingspage.h
    settings/dolphinsettingsdialog.h
    settings/navigation/navigationsettingspage.h
    settings/contextmenu/contextmenusettingspage.h
    settings/settingspagebase.h
    settings/serviceitemdelegate.h
    settings/servicemodel.h
    settings/startup/startupsettingspage.h
    settings/trash/trashsettingspage.h
    settings/viewmodes/dolphinfontrequester.h
    settings/viewmodes/viewsettingspage.h
    settings/viewmodes/viewmodesettings.h
    settings/viewmodes/viewsettingstab.h
    statusbar/dolphinstatusbar.h
    statusbar/mountpointobserver.h
    statusbar/mountpointobservercache.h
    statusbar/spaceinfoobserver.h
    statusbar/statusbarspaceinfo.h
    views/zoomlevelinfo.h
    dolphindebug.h
    global.h
)

if(HAVE_BALOO)
    target_sources(dolphinstatic PRIVATE
        panels/information/informationpanel.cpp
        panels/information/informationpanelcontent.cpp
        panels/information/pixmapviewer.cpp
        panels/information/phononwidget.cpp
        panels/information/informationpanel.h
        panels/information/informationpanelcontent.h
        panels/information/pixmapviewer.h
        panels/information/phononwidget.h
    )
endif()

if(HAVE_KUSERFEEDBACK)
    target_sources(dolphinstatic PRIVATE
        userfeedback/dolphinfeedbackprovider.cpp
        userfeedback/settingsdatasource.cpp
        userfeedback/placesdatasource.cpp
        settings/userfeedback/userfeedbacksettingspage.cpp
        userfeedback/dolphinfeedbackprovider.h
        userfeedback/settingsdatasource.h
        userfeedback/placesdatasource.h
        settings/userfeedback/userfeedbacksettingspage.h
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

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/dolphinmainwindow.h org.kde.DolphinMainWindow.xml)
qt_add_dbus_adaptor(dolphin_dbus_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.DolphinMainWindow.xml dolphinmainwindow.h DolphinMainWindow)
qt_add_dbus_interface(dolphin_dbus_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.DolphinMainWindow.xml dolphinmainwindowinterface)
qt_add_dbus_interface(dolphin_dbus_SRCS panels/terminal/org.kde.KIOFuse.VFS.xml kiofuse_interface)

target_sources(dolphinstatic PRIVATE
    ${dolphin_dbus_SRCS}
)

target_include_directories(dolphinstatic SYSTEM PRIVATE ${PHONON_INCLUDES})
target_link_libraries(dolphinstatic
    dolphinprivate
    KF6::CoreAddons
    KF6::KCMUtils
    KF6::DBusAddons
    KF6::Notifications
    Phonon::phonon4qt${QT_MAJOR_VERSION}
)

if (HAVE_KACTIVITIES)
    target_link_libraries(
        dolphinstatic
        KF6::Activities
    )
endif()

if (HAVE_KUSERFEEDBACK)
    target_link_libraries(
        dolphinstatic
        KUserFeedbackCoreQt6
        KUserFeedbackWidgetsQt6
    )
endif()

add_executable(dolphin)

target_sources(dolphin PRIVATE
    dbusinterface.cpp
    dbusinterface.h
    main.cpp
)

if(FLATPAK)
    target_compile_definitions(dolphin PRIVATE FLATPAK)
endif()

# Sets the icon on Windows and OSX
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*system-file-manager.png")
ecm_add_app_icon(dolphin ICONS ${ICONS_SRCS})

target_link_libraries(dolphin
    PRIVATE
    dolphinprivate
    dolphinstatic
    KF6::Crash
)

if (HAVE_X11)
    if (QT_MAJOR_VERSION STREQUAL "5")
        target_link_libraries(dolphin PRIVATE Qt{QT_MAJOR_VERSION}::X11Extras)
    else()
        target_link_libraries(dolphin PRIVATE Qt::GuiPrivate)
    endif()
endif()

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
    add_library(kcm_dolphinviewmodes MODULE)
    add_library(kcm_dolphinnavigation MODULE)
    add_library(kcm_dolphingeneral MODULE)

    target_sources(kcm_dolphinviewmodes PRIVATE
        settings/kcm/kcmdolphinviewmodes.cpp
        settings/viewmodes/dolphinfontrequester.cpp
        settings/viewmodes/viewmodesettings.cpp
        settings/viewmodes/viewsettingstab.cpp
        views/zoomlevelinfo.cpp
        settings/kcm/kcmdolphinviewmodes.h
        settings/viewmodes/dolphinfontrequester.h
        settings/viewmodes/viewmodesettings.h
        settings/viewmodes/viewsettingstab.h
        views/zoomlevelinfo.h)

    target_sources(kcm_dolphinnavigation PRIVATE
        settings/kcm/kcmdolphinnavigation.cpp
        settings/navigation/navigationsettingspage.cpp
        settings/settingspagebase.cpp
        settings/kcm/kcmdolphinnavigation.h
        settings/navigation/navigationsettingspage.h
        settings/settingspagebase.h
    )

    target_sources(kcm_dolphingeneral PRIVATE
        settings/kcm/kcmdolphingeneral.cpp
        settings/general/behaviorsettingspage.cpp
        settings/general/previewssettingspage.cpp
        settings/general/confirmationssettingspage.cpp
        settings/settingspagebase.cpp
        settings/serviceitemdelegate.cpp
        settings/servicemodel.cpp
        settings/kcm/kcmdolphingeneral.h
        settings/general/behaviorsettingspage.h
        settings/general/previewssettingspage.h
        settings/general/confirmationssettingspage.h
        settings/settingspagebase.h
        settings/serviceitemdelegate.h
        settings/servicemodel.h
    )

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

install( FILES settings/dolphin_detailsmodesettings.upd
         DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR} )

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
