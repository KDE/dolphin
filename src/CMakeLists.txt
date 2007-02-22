
add_subdirectory( pics )
add_subdirectory( servicemenus )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(dolphin_SRCS
   applyviewpropsjob.cpp
   bookmarkselector.cpp
   bookmarkssettingspage.cpp
   bookmarkssidebarpage.cpp
   detailsviewsettingspage.cpp
   dolphinapplication.cpp
   dolphincontroller.cpp
   dolphinmainwindow.cpp
   dolphinnewmenu.cpp
   dolphinview.cpp
   dolphinstatusbar.cpp
   dolphindirlister.cpp
   dolphindetailsview.cpp
   dolphiniconsview.cpp
   dolphinsettings.cpp
   dolphincontextmenu.cpp
   dolphinsortfilterproxymodel.cpp
   dolphinsettingsdialog.cpp
   editbookmarkdialog.cpp
   filterbar.cpp
   generalsettingspage.cpp
   generalviewsettingspage.cpp
   iconsviewsettingspage.cpp
   infosidebarpage.cpp
   main.cpp
   metadataloader.cpp
   protocolcombo.cpp
   pixmapviewer.cpp
   renamedialog.cpp
   settingspagebase.cpp
   sidebarpage.cpp
   statusbarspaceinfo.cpp
   statusbarmessagelabel.cpp
   urlbutton.cpp
   urlnavigator.cpp
   urlnavigatorbutton.cpp
   viewpropertiesdialog.cpp
   viewproperties.cpp
   viewsettingspage.cpp
   viewpropsprogressinfo.cpp )

kde4_automoc(${dolphin_SRCS})

kde4_add_kcfg_files(dolphin_SRCS
   dolphin_directoryviewpropertysettings.kcfgc
   dolphin_generalsettings.kcfgc
   dolphin_iconsmodesettings.kcfgc
   dolphin_detailsmodesettings.kcfgc )

qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.Application.xml
   dolphinapplication.h DolphinApplication)
qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.MainWindow.xml
   dolphinmainwindow.h DolphinMainWindow)
kde4_add_executable(dolphin ${dolphin_SRCS})

#set(KDE4_KMETADATA kmetadata)
target_link_libraries(dolphin  ${KDE4_KDECORE_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KMETADATA} konq)

install(TARGETS dolphin DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES  dolphin_directoryviewpropertysettings.kcfg dolphin_generalsettings.kcfg dolphin_iconsmodesettings.kcfg dolphin_detailsmodesettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )

kde4_install_icons( ${ICON_INSTALL_DIR} )
