
add_subdirectory( pics )
add_subdirectory( servicemenus )
add_subdirectory( tests )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${BLITZ_INCLUDES} )

########### next target ###############

set(dolphinprivate_LIB_SRCS
    dolphincontroller.cpp
    dolphindetailsview.cpp
    dolphiniconsview.cpp
    dolphincolumnview.cpp
    kcategorizedview.cpp
    kcategorydrawer.cpp
    dolphinmodel.cpp
    dolphinsettings.cpp
    dolphincategorydrawer.cpp
    viewproperties.cpp
    dolphinsortfilterproxymodel.cpp
    renamedialog.cpp
    dolphinview.cpp
    )

kde4_add_kcfg_files(dolphinprivate_LIB_SRCS
    dolphin_columnmodesettings.kcfgc
    dolphin_directoryviewpropertysettings.kcfgc
    dolphin_detailsmodesettings.kcfgc
    dolphin_iconsmodesettings.kcfgc
    dolphin_generalsettings.kcfgc)



kde4_add_library(dolphinprivate SHARED ${dolphinprivate_LIB_SRCS})

find_package(Nepomuk)
macro_bool_to_01(Nepomuk_FOUND HAVE_NEPOMUK)

configure_file(config-nepomuk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-nepomuk.h )

target_link_libraries(dolphinprivate ${KDE4_KFILE_LIBS} konq ${BLITZ_LIBRARIES})
if (Nepomuk_FOUND)
  target_link_libraries(dolphinprivate ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)


set_target_properties(dolphinprivate PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS dolphinprivate  DESTINATION ${LIB_INSTALL_DIR} )


##########################################

set(dolphinpart_SRCS
   dolphinpart.cpp
)


kde4_add_plugin(dolphinpart ${dolphinpart_SRCS})

target_link_libraries(dolphinpart dolphinprivate)

install(TARGETS dolphinpart DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES dolphinpart.rc DESTINATION ${DATA_INSTALL_DIR}/dolphinpart )
install(FILES dolphinpart.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


##########################################

set(dolphin_SRCS
   applyviewpropsjob.cpp
   columnviewsettingspage.cpp
   detailsviewsettingspage.cpp
   dolphinapplication.cpp
   dolphinmainwindow.cpp
   dolphinnewmenu.cpp
   dolphinviewcontainer.cpp
   dolphinstatusbar.cpp
   dolphindirlister.cpp
   dolphincontextmenu.cpp
   dolphinsettingsdialog.cpp
   filterbar.cpp
   generalsettingspage.cpp
   generalviewsettingspage.cpp
   iconsizedialog.cpp
   iconsviewsettingspage.cpp
   infosidebarpage.cpp
   main.cpp
   metadatawidget.cpp
   pixmapviewer.cpp
   settingspagebase.cpp
   sidebarpage.cpp
   statusbarspaceinfo.cpp
   statusbarmessagelabel.cpp
   terminalsidebarpage.cpp
   treeviewcontextmenu.cpp
   treeviewsidebarpage.cpp
   sidebartreeview.cpp
   viewpropertiesdialog.cpp
   viewsettingspage.cpp
   viewpropsprogressinfo.cpp )


qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.Application.xml
   dolphinapplication.h DolphinApplication)
qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.MainWindow.xml
   dolphinmainwindow.h DolphinMainWindow)
kde4_add_executable(dolphin ${dolphin_SRCS})

target_link_libraries(dolphin  ${KDE4_KDEPRINT_LIBS} ${KDE4_KFILE_LIBS} konq dolphinprivate)

if (Nepomuk_FOUND)
  target_link_libraries(dolphin ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)

install(TARGETS dolphin DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES  dolphin_directoryviewpropertysettings.kcfg dolphin_generalsettings.kcfg dolphin_columnmodesettings.kcfg dolphin_iconsmodesettings.kcfg dolphin_detailsmodesettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )
