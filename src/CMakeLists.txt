
add_subdirectory( pics )
add_subdirectory( servicemenus )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_definitions (-DQT3_SUPPORT)

########### next target ###############

set(dolphinprivate_LIB_SRCS
	dolphincontroller.cpp
	dolphindetailsview.cpp
	dolphiniconsview.cpp
	dolphinsettings.cpp
	viewproperties.cpp
	dolphinsortfilterproxymodel.cpp
	)

kde4_add_kcfg_files(dolphinprivate_LIB_SRCS
   dolphin_columnmodesettings.kcfgc
   dolphin_directoryviewpropertysettings.kcfgc
   dolphin_detailsmodesettings.kcfgc
   dolphin_iconsmodesettings.kcfgc
   dolphin_generalsettings.kcfgc)


kde4_automoc(${dolphinprivate_LIB_SRCS})

kde4_add_library(dolphinprivate SHARED ${dolphinprivate_LIB_SRCS})

target_link_libraries(dolphinprivate ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} konq)

set_target_properties(dolphinprivate PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS dolphinprivate  DESTINATION ${LIB_INSTALL_DIR} )


##########################################

set(dolphin_SRCS
   applyviewpropsjob.cpp
   columnviewsettingspage.cpp
   detailsviewsettingspage.cpp
   dolphinapplication.cpp
   dolphincolumnview.cpp
   dolphinmainwindow.cpp
   dolphinnewmenu.cpp
   dolphinview.cpp
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
   renamedialog.cpp
   settingspagebase.cpp
   sidebarpage.cpp
   statusbarspaceinfo.cpp
   statusbarmessagelabel.cpp
   treeviewcontextmenu.cpp
   treeviewsidebarpage.cpp
   sidebartreeview.cpp
   viewpropertiesdialog.cpp
   viewsettingspage.cpp
   viewpropsprogressinfo.cpp )

kde4_automoc(${dolphin_SRCS})

qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.Application.xml
   dolphinapplication.h DolphinApplication)
qt4_add_dbus_adaptor(dolphin_SRCS
   org.kde.dolphin.MainWindow.xml
   dolphinmainwindow.h DolphinMainWindow)
kde4_add_executable(dolphin ${dolphin_SRCS})
# find Nepomuk's libkmetadata
find_path(KMETADATA_INCLUDE_DIR NAMES kmetadata/kmetadata.h PATHS ${KDE4_INCLUDE_DIR})
find_library(KMETADATA_LIBRARY NAMES kmetadata kmetadatatools PATHS ${KDE4_LIB_DIR})
find_library(KMETADATA_TOOLS_LIBRARY NAMES kmetadatatools PATHS ${KDE4_LIB_DIR})
if(KMETADATA_INCLUDE_DIR AND KMETADATA_LIBRARY AND KMETADATA_TOOLS_LIBRARY)
set(KDE4_KMETADATA kmetadata kmetadatatools)
set(HAVE_KMETADATA 1)
else(KMETADATA_INCLUDE_DIR AND KMETADATA_LIBRARY)
set(KDE4_KMETADATA "")
endif(KMETADATA_INCLUDE_DIR AND KMETADATA_LIBRARY AND KMETADATA_TOOLS_LIBRARY)
configure_file(config-kmetadata.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kmetadata.h )
target_link_libraries(dolphin  ${KDE4_KDECORE_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_KMETADATA} konq dolphinprivate)

install(TARGETS dolphin DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES  dolphin_directoryviewpropertysettings.kcfg dolphin_generalsettings.kcfg dolphin_columnmodesettings.kcfg dolphin_iconsmodesettings.kcfg dolphin_detailsmodesettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )

kde4_install_icons( ${ICON_INSTALL_DIR} )
