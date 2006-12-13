
add_subdirectory( pics )
add_subdirectory( servicemenus )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(dolphin_SRCS
   main.cpp
   dolphinapplication.cpp
   dolphinmainwindow.cpp
   dolphinview.cpp
   urlnavigator.cpp
   urlnavigatorbutton.cpp
   viewpropertiesdialog.cpp
   dolphinstatusbar.cpp
   dolphindirlister.cpp
   viewproperties.cpp
   dolphindetailsview.cpp
   dolphiniconsview.cpp
   dolphinsettings.cpp
   bookmarkselector.cpp
   urlbutton.cpp
   dolphincontextmenu.cpp
   undomanager.cpp
   progressindicator.cpp
   iconsviewsettingspage.cpp
   pixmapviewer.cpp
   dolphinsettingsdialog.cpp
   viewsettingspage.cpp
   detailsviewsettingspage.cpp
   statusbarmessagelabel.cpp
   generalsettingspage.cpp
   bookmarkssettingspage.cpp
   editbookmarkdialog.cpp
   settingspagebase.cpp
   sidebarpage.cpp
   bookmarkssidebarpage.cpp
   infosidebarpage.cpp
   statusbarspaceinfo.cpp
   renamedialog.cpp
   filterbar.cpp
   protocolcombo.cpp
   viewpropsprogressinfo.cpp
   applyviewpropsjob.cpp )

kde4_automoc(${dolphin_SRCS})

kde4_add_kcfg_files(dolphin_SRCS
   directoryviewpropertysettings.kcfgc
   generalsettings.kcfgc
   iconsmodesettings.kcfgc
   detailsmodesettings.kcfgc )

kde4_add_executable(dolphin ${dolphin_SRCS})

target_link_libraries(dolphin  ${KDE4_KDECORE_LIBS} ${KDE4_KDEPRINT_LIBS} )

install(TARGETS dolphin DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES  directoryviewpropertysettings.kcfg generalsettings.kcfg iconsmodesettings.kcfg detailsmodesettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )

kde4_install_icons( ${ICON_INSTALL_DIR} )
