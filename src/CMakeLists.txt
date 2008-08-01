
add_subdirectory( pics )

find_package(QImageBlitz REQUIRED)

macro_optional_find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk" "http://www.kde.org" FALSE "" "For adding desktop-wide tagging support to dolphin")
macro_bool_to_01(Nepomuk_FOUND HAVE_NEPOMUK)
configure_file(config-nepomuk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-nepomuk.h )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${QIMAGEBLITZ_INCLUDES} )

if (Nepomuk_FOUND)
  # Yes, Soprano includes is what we need here
  include_directories( ${SOPRANO_INCLUDE_DIR} ${NEPOMUK_INCLUDE_DIR} )
endif (Nepomuk_FOUND)

########### next target ###############

set(dolphinprivate_LIB_SRCS
    dolphincontroller.cpp
    dolphindetailsview.cpp
    dolphiniconsview.cpp
    dolphincolumnview.cpp
    dolphincolumnwidget.cpp
    dolphindropcontroller.cpp
    dolphinsortfilterproxymodel.cpp
    draganddrophelper.cpp
    dolphinmodel.cpp
    dolphinsettings.cpp
    dolphincategorydrawer.cpp
    dolphinview.cpp
    dolphinviewactionhandler.cpp
    iconmanager.cpp
    ktooltip.cpp
    kballoontipdelegate.cpp
    kformattedballoontipdelegate.cpp
    renamedialog.cpp
    selectiontoggle.cpp
    selectionmanager.cpp
    tooltipmanager.cpp
    viewproperties.cpp
    viewpropertiesdialog.cpp
    viewpropsprogressinfo.cpp
    additionalinfodialog.cpp
    applyviewpropsjob.cpp
    )

kde4_add_kcfg_files(dolphinprivate_LIB_SRCS
    dolphin_columnmodesettings.kcfgc
    dolphin_directoryviewpropertysettings.kcfgc
    dolphin_detailsmodesettings.kcfgc
    dolphin_iconsmodesettings.kcfgc
    dolphin_generalsettings.kcfgc)

kde4_add_library(dolphinprivate SHARED ${dolphinprivate_LIB_SRCS})

target_link_libraries(dolphinprivate ${KDE4_KFILE_LIBS} konq ${KDE4_KIO_LIBS} ${QIMAGEBLITZ_LIBRARIES} ${X11_LIBRARIES})
if(X11_Xrender_FOUND)
  target_link_libraries(dolphinprivate ${X11_Xrender_LIB} )
endif(X11_Xrender_FOUND)
if (Nepomuk_FOUND)
  target_link_libraries(dolphinprivate ${NEPOMUK_LIBRARIES} ${SOPRANO_LIBRARIES})
endif (Nepomuk_FOUND)


set_target_properties(dolphinprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS dolphinprivate  ${INSTALL_TARGETS_DEFAULT_ARGS})


##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
)


kde4_add_plugin(dolphinpart ${dolphinpart_SRCS})

target_link_libraries(dolphinpart dolphinprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS dolphinpart DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES dolphinpart.rc DESTINATION ${DATA_INSTALL_DIR}/dolphinpart)
install(FILES dolphinpart.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

##########################################

set(dolphin_SRCS
    columnviewsettingspage.cpp
    commentwidget.cpp
    commenteditwidget.cpp
    detailsviewsettingspage.cpp
    dolphinapplication.cpp
    dolphinfontrequester.cpp
    dolphinmainwindow.cpp
    dolphinnewmenu.cpp
    dolphinviewcontainer.cpp
    dolphinstatusbar.cpp
    dolphindirlister.cpp
    dolphincontextmenu.cpp
    dolphinfileplacesview.cpp
    dolphinsettingsdialog.cpp
    draganddrophelper.cpp
    filterbar.cpp
    generalsettingspage.cpp
    generalviewsettingspage.cpp
    iconsizedialog.cpp
    iconsviewsettingspage.cpp
    infosidebarpage.cpp
	ktreeview.cpp
    main.cpp
    metadatawidget.cpp
    metatextlabel.cpp
    pixmapviewer.cpp
    settingspagebase.cpp
    sidebarpage.cpp
    startupsettingspage.cpp
    statusbarspaceinfo.cpp
    statusbarmessagelabel.cpp
    treeviewcontextmenu.cpp
    treeviewsidebarpage.cpp
    sidebartreeview.cpp
    viewsettingspage.cpp
    viewsettingspagebase.cpp
    )

kde4_add_kcfg_files(dolphin_SRCS
    dolphin_folderspanelsettings.kcfgc)

if(Nepomuk_FOUND)
set(dolphin_SRCS
  ${dolphin_SRCS}
  nepomukmassupdatejob.cpp
  tagcloud/tagcloud.cpp
  tagcloud/resourcetaggingwidget.cpp
  tagcloud/taggingpopup.cpp
  tagcloud/newtagdialog.cpp
)
kde4_add_ui_files(dolphin_SRCS tagcloud/newtagdialog.ui)

endif(Nepomuk_FOUND)

if(NOT WIN32)
   set(dolphin_SRCS ${dolphin_SRCS} terminalsidebarpage.cpp)
endif(NOT WIN32)

qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.Application.xml
   dolphinapplication.h DolphinApplication)
qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.MainWindow.xml
   dolphinmainwindow.h DolphinMainWindow)

kde4_add_app_icon(dolphin_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

kde4_add_executable(dolphin ${dolphin_SRCS})

target_link_libraries(dolphin  ${KDE4_KDEPRINT_LIBS} ${KDE4_KFILE_LIBS} konq dolphinprivate)

if (Nepomuk_FOUND)
  target_link_libraries(dolphin ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)

install(TARGETS dolphin ${INSTALL_TARGETS_DEFAULT_ARGS})

##########################################

set(kcm_dolphin_PART_SRCS
    kcmdolphin.cpp
    columnviewsettingspage.cpp
    detailsviewsettingspage.cpp
    dolphinfontrequester.cpp
#    dolphinsettings.cpp
    generalviewsettingspage.cpp
    iconsizedialog.cpp
    iconsviewsettingspage.cpp
    pixmapviewer.cpp
    viewsettingspagebase.cpp
#    viewproperties.cpp
)

kde4_add_kcfg_files(kcm_dolphin_PART_SRCS
    dolphin_columnmodesettings.kcfgc
    dolphin_directoryviewpropertysettings.kcfgc
    dolphin_detailsmodesettings.kcfgc
    dolphin_iconsmodesettings.kcfgc
    dolphin_generalsettings.kcfgc)

kde4_add_plugin(kcm_dolphin ${kcm_dolphin_PART_SRCS})

target_link_libraries(kcm_dolphin  ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} dolphinprivate)
if (Nepomuk_FOUND)
  target_link_libraries(kcm_dolphin ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)

install(TARGETS kcm_dolphin DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES  dolphin_directoryviewpropertysettings.kcfg dolphin_generalsettings.kcfg dolphin_columnmodesettings.kcfg dolphin_iconsmodesettings.kcfg dolphin_detailsmodesettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )
install( FILES kcmdolphin.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
