
find_package(QImageBlitz REQUIRED)

macro_optional_find_package(SharedDesktopOntologies)
macro_log_feature(SHAREDDESKTOPONTOLOGIES_FOUND "Shared desktop ontologies" "Desktop ontologies" "http://oscaf.sourceforge.net" FALSE "" "Ontologies necessary for the Nepomuk semantic desktop.")

macro_optional_find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk" "http://www.kde.org" FALSE "" "For adding desktop-wide tagging support to dolphin")

if(SHAREDDESKTOPONTOLOGIES_FOUND AND Nepomuk_FOUND)
    set(HAVE_NEPOMUK TRUE)
endif(SHAREDDESKTOPONTOLOGIES_FOUND AND Nepomuk_FOUND)

configure_file(config-nepomuk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-nepomuk.h )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${QIMAGEBLITZ_INCLUDES} )

if (HAVE_NEPOMUK)
  # Yes, Soprano includes is what we need here
  include_directories( ${SOPRANO_INCLUDE_DIR} ${NEPOMUK_INCLUDE_DIR} )
endif (HAVE_NEPOMUK)

########### next target ###############

set(dolphinprivate_LIB_SRCS
    dolphincontroller.cpp
    dolphindetailsview.cpp
    dolphiniconsview.cpp
    dolphincolumnview.cpp
    dolphincolumnviewcontainer.cpp
    dolphindirlister.cpp
    dolphinfileitemdelegate.cpp
    dolphinmodel.cpp
    dolphinnewmenuobserver.cpp
    dolphinsortfilterproxymodel.cpp
    dolphincategorydrawer.cpp
    dolphinview.cpp
    dolphinviewactionhandler.cpp
    dolphinviewautoscroller.cpp
    dolphinremoteencoding.cpp
    dolphindetailsviewexpander.cpp
    draganddrophelper.cpp
    folderexpander.cpp
    renamedialog.cpp
    selectiontoggle.cpp
    selectionmanager.cpp
    settings/additionalinfodialog.cpp
    settings/applyviewpropsjob.cpp
    settings/dolphinsettings.cpp
    settings/viewpropertiesdialog.cpp
    settings/viewpropsprogressinfo.cpp
    panels/information/kmetadatawidget.cpp
    panels/information/kmetadataconfigurationdialog.cpp
    tooltips/ktooltip.cpp
    tooltips/ktooltipwindow.cpp
    tooltips/tooltipmanager.cpp
    versioncontrol/updateitemstatesthread.cpp
    versioncontrol/versioncontrolobserver.cpp
    viewextensionsfactory.cpp
    viewproperties.cpp
    zoomlevelinfo.cpp
)

if(HAVE_NEPOMUK)
    set(dolphinprivate_LIB_SRCS
        ${dolphinprivate_LIB_SRCS}
        panels/information/kcommentwidget.cpp
        panels/information/kedittagsdialog.cpp
        panels/information/kloadmetadatathread.cpp
        panels/information/nepomukmassupdatejob.cpp
        panels/information/ktaggingwidget.cpp
    )
endif(HAVE_NEPOMUK)

kde4_add_kcfg_files(dolphinprivate_LIB_SRCS
    settings/dolphin_columnmodesettings.kcfgc
    settings/dolphin_directoryviewpropertysettings.kcfgc
    settings/dolphin_detailsmodesettings.kcfgc
    settings/dolphin_iconsmodesettings.kcfgc
    settings/dolphin_generalsettings.kcfgc
    settings/dolphin_versioncontrolsettings.kcfgc
)

kde4_add_library(dolphinprivate SHARED ${dolphinprivate_LIB_SRCS})

target_link_libraries(dolphinprivate ${KDE4_KFILE_LIBS} konq ${KDE4_KNEWSTUFF3_LIBS} ${QIMAGEBLITZ_LIBRARIES} ${X11_LIBRARIES})
if(X11_Xrender_FOUND)
  target_link_libraries(dolphinprivate ${X11_Xrender_LIB} )
endif(X11_Xrender_FOUND)
if (HAVE_NEPOMUK)
  target_link_libraries(dolphinprivate ${NEPOMUK_LIBRARIES} ${NEPOMUK_QUERY_LIBRARIES} ${SOPRANO_LIBRARIES})
endif (HAVE_NEPOMUK)


set_target_properties(dolphinprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS dolphinprivate  ${INSTALL_TARGETS_DEFAULT_ARGS})


##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
)

set(fileviewsvnplugin_SRCS
    versioncontrol/fileviewsvnplugin.cpp
)

kde4_add_plugin(dolphinpart ${dolphinpart_SRCS})
kde4_add_plugin(fileviewsvnplugin ${fileviewsvnplugin_SRCS})

target_link_libraries(dolphinpart dolphinprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS})
target_link_libraries(fileviewsvnplugin ${KDE4_KIO_LIBS} konq)

install(TARGETS dolphinpart DESTINATION ${PLUGIN_INSTALL_DIR})
install(TARGETS fileviewsvnplugin DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES dolphinpart.rc DESTINATION ${DATA_INSTALL_DIR}/dolphinpart)
install(FILES dolphinpart.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES versioncontrol/fileviewsvnplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES versioncontrol/fileviewversioncontrolplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

##########################################

set(dolphin_SRCS
    dolphinapplication.cpp
    dolphinmainwindow.cpp
    dolphinnewmenu.cpp
    dolphinviewcontainer.cpp
    dolphindirlister.cpp
    dolphincontextmenu.cpp
    filterbar.cpp
    main.cpp
    pixmapviewer.cpp
    panels/information/informationpanel.cpp
    panels/information/informationpanelcontent.cpp
    panels/information/kmetadatawidget.cpp
    panels/information/kmetadataconfigurationdialog.cpp
    panels/information/phononwidget.cpp
    panels/folders/ktreeview.cpp
    panels/places/placespanel.cpp
    panels/panel.cpp
    panels/folders/treeviewcontextmenu.cpp
    panels/folders/folderspanel.cpp
    panels/folders/paneltreeview.cpp
    search/dolphinsearchbox.cpp
    settings/behaviorsettingspage.cpp
    settings/columnviewsettingspage.cpp
    settings/contextmenusettingspage.cpp
    settings/dolphinfontrequester.cpp
    settings/dolphinsettingsdialog.cpp
    settings/detailsviewsettingspage.cpp
    settings/generalsettingspage.cpp
    settings/iconsizegroupbox.cpp
    settings/iconsviewsettingspage.cpp
    settings/navigationsettingspage.cpp
    settings/previewssettingspage.cpp
    settings/servicessettingspage.cpp
    settings/settingspagebase.cpp
    settings/startupsettingspage.cpp
    settings/statusbarsettingspage.cpp
    settings/trashsettingspage.cpp
    settings/viewsettingspage.cpp
    settings/viewsettingspagebase.cpp
    statusbar/dolphinstatusbar.cpp
    statusbar/statusbarspaceinfo.cpp
    statusbar/statusbarmessagelabel.cpp
    zoomlevelinfo.cpp
)

kde4_add_kcfg_files(dolphin_SRCS
    panels/folders/dolphin_folderspanelsettings.kcfgc
    panels/information/dolphin_informationpanelsettings.kcfgc
    search/dolphin_searchsettings.kcfgc
)

if(HAVE_NEPOMUK)
    set(dolphin_SRCS
        ${dolphin_SRCS}
        panels/information/kcommentwidget.cpp
        panels/information/kedittagsdialog.cpp
        panels/information/kloadmetadatathread.cpp
        panels/information/nepomukmassupdatejob.cpp
        panels/information/ktaggingwidget.cpp
        search/dolphinsearchoptionsconfigurator.cpp
        search/searchcriteriondescription.cpp
        search/searchcriterionselector.cpp
        search/searchcriterionvalue.cpp
    )
endif(HAVE_NEPOMUK)

if(HAVE_NEPOMUK)
    include(SopranoAddOntology)
    soprano_add_ontology(dolphin_SRCS "${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig" "NFO" "Nepomuk::Vocabulary" "trig")
endif(HAVE_NEPOMUK)

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

kde4_add_executable(dolphin ${dolphin_SRCS})

target_link_libraries(dolphin
    ${KDE4_KDEPRINT_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KUTILS_LIBRARY}
    konq
    dolphinprivate
    knewstuff3
    ${KDE4_PHONON_LIBS}
)

if (HAVE_NEPOMUK)
    target_link_libraries(dolphin
        ${NEPOMUK_LIBRARIES}
        ${SOPRANO_LIBRARIES}
        ${NEPOMUK_QUERY_LIBRARIES}
    )
endif (HAVE_NEPOMUK)

install(TARGETS dolphin ${INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

set(kcm_dolphinviewmodes_PART_SRCS
    kcm/kcmdolphinviewmodes.cpp
    pixmapviewer.cpp
    settings/columnviewsettingspage.cpp
    settings/detailsviewsettingspage.cpp
    settings/dolphinfontrequester.cpp
    settings/iconsizegroupbox.cpp
    settings/iconsviewsettingspage.cpp
    settings/viewsettingspagebase.cpp
    zoomlevelinfo.cpp)

set(kcm_dolphinnavigation_PART_SRCS
    kcm/kcmdolphinnavigation.cpp
    settings/navigationsettingspage.cpp
    settings/settingspagebase.cpp)

set(kcm_dolphinservices_PART_SRCS
    kcm/kcmdolphinservices.cpp
    settings/servicessettingspage.cpp
    settings/settingspagebase.cpp)

set(kcm_dolphingeneral_PART_SRCS
    kcm/kcmdolphingeneral.cpp
    panels/information/kmetadataconfigurationdialog.cpp
    panels/information/kmetadatawidget.cpp
    settings/behaviorsettingspage.cpp
    settings/previewssettingspage.cpp
    settings/contextmenusettingspage.cpp
    settings/settingspagebase.cpp)
if (HAVE_NEPOMUK)
    set(kcm_dolphingeneral_PART_SRCS
        ${kcm_dolphingeneral_PART_SRCS}
        panels/information/kcommentwidget.cpp
        panels/information/kedittagsdialog.cpp
        panels/information/kloadmetadatathread.cpp
        panels/information/nepomukmassupdatejob.cpp
        panels/information/ktaggingwidget.cpp
    )
endif (HAVE_NEPOMUK)

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
    settings/dolphin_generalsettings.kcfgc)

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
if (HAVE_NEPOMUK)
  target_link_libraries(kcm_dolphinviewmodes ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphinnavigation ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphinservices ${NEPOMUK_LIBRARIES})
  target_link_libraries(kcm_dolphingeneral ${NEPOMUK_LIBRARIES})
endif (HAVE_NEPOMUK)

install(TARGETS kcm_dolphinviewmodes DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinnavigation DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphinservices DESTINATION ${PLUGIN_INSTALL_DIR} )
install(TARGETS kcm_dolphingeneral DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES dolphin.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES settings/dolphin_directoryviewpropertysettings.kcfg
               settings/dolphin_generalsettings.kcfg
               settings/dolphin_columnmodesettings.kcfg
               settings/dolphin_iconsmodesettings.kcfg
               settings/dolphin_detailsmodesettings.kcfg
               settings/dolphin_versioncontrolsettings.kcfg
         DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )
install( FILES search/dolphinsearchcommands.desktop DESTINATION ${DATA_INSTALL_DIR}/dolphin )
install( FILES kcm/kcmdolphinviewmodes.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kcm/kcmdolphinnavigation.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kcm/kcmdolphinservices.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kcm/kcmdolphingeneral.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES khns/servicemenu.knsrc DESTINATION ${CONFIG_INSTALL_DIR} )
install( PROGRAMS khns/servicemenuinstallation DESTINATION ${BIN_INSTALL_DIR} )
install( PROGRAMS khns/servicemenudeinstallation DESTINATION ${BIN_INSTALL_DIR} )
