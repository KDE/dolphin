
add_subdirectory( pics )
add_subdirectory( servicemenus )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(dolphin_SRCS 
   main.cpp 
   dolphin.cpp 
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
   itemeffectsmanager.cpp 
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
   sidebar.cpp 
   sidebarpage.cpp 
   bookmarkssidebarpage.cpp 
   infosidebarpage.cpp 
   statusbarspaceinfo.cpp 
   renamedialog.cpp 
   filterbar.cpp 
   protocolcombo.cpp )

kde4_automoc(${dolphin_SRCS})

kde4_add_kcfg_files(dolphin_SRCS 
   directoryviewpropertysettings.kcfgc
   generalsettings.kcfgc
   iconsmodesettings.kcfgc
   detailsmodesettings.kcfgc
   previewsmodesettings.kcfgc
   sidebarsettings.kcfgc )

kde4_add_executable(dolphin ${dolphin_SRCS})

target_link_libraries(dolphin  ${KDE4_KDECORE_LIBS} ${KDE4_KDEPRINT_LIBS} )

install(TARGETS dolphin DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  dolphin.desktop DESTINATION ${XDG_APPS_DIR} )
install( FILES  directoryviewpropertysettings.kcfg generalsettings.kcfg  				iconsmodesettings.kcfg detailsmodesettings.kcfg previewsmodesettings.kcfg  				sidebarsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  dolphinui.rc DESTINATION ${DATA_INSTALL_DIR}/dolphin )
install( FILES  DESTINATION ${APPLNK_INSTALL_DIR}/System )

kde4_install_icons( ${ICON_INSTALL_DIR} )



#original Makefile.am contents follow:

### Makefile.am for dolphin
#
#SUBDIRS = pics servicemenus
#
## this is the program that gets installed.  it's name is used for all
## of the other Makefile.am variables
#bin_PROGRAMS = dolphin
#
## set the include path for X, qt and KDE
#INCLUDES         = $(all_includes)
#
## the library search path.
#dolphin_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#
## the libraries to link against.
#dolphin_LDADD   = $(LIB_KFILE) $(LIB_KDEPRINT)
#
## which sources should be compiled for dolphin
#dolphin_SOURCES = main.cpp dolphin.cpp dolphinview.cpp urlnavigator.cpp directoryviewpropertysettings.kcfgc \
#		urlnavigatorbutton.cpp viewpropertiesdialog.cpp dolphinstatusbar.cpp dolphindirlister.cpp \
#		viewproperties.cpp dolphindetailsview.cpp dolphiniconsview.cpp dolphinsettings.cpp \
#		bookmarkselector.cpp urlbutton.cpp itemeffectsmanager.cpp \
#		dolphincontextmenu.cpp undomanager.cpp progressindicator.cpp \
#		iconsviewsettingspage.cpp pixmapviewer.cpp dolphinsettingsdialog.cpp \
#		viewsettingspage.cpp detailsviewsettingspage.cpp statusbarmessagelabel.cpp \
#		generalsettingspage.cpp bookmarkssettingspage.cpp \
#		editbookmarkdialog.cpp settingspagebase.cpp sidebar.cpp sidebarpage.cpp \
#		bookmarkssidebarpage.cpp infosidebarpage.cpp statusbarspaceinfo.cpp renamedialog.cpp \
#		filterbar.cpp protocolcombo.cpp generalsettings.kcfgc iconsmodesettings.kcfgc \
#		detailsmodesettings.kcfgc previewsmodesettings.kcfgc sidebarsettings.kcfgc
#
## these are the headers for your project
#noinst_HEADERS = dolphin.h dolphinview.h urlnavigator.h urlnavigatorbutton.h \
#		viewpropertiesdialog.h dolphinstatusbar.h dolphindirlister.h viewproperties.h \
#		dolphindetailsview.h dolphiniconsview.h dolphinsettings.h bookmarkselector.h \
#		urlbutton.h itemeffectsmanager.h dolphincontextmenu.h undomanager.h \
#		iconsviewsettingspage.h pixmapviewer.h \
#		dolphinsettingsdialog.h detailsviewsettingspage.h viewsettingspage.h statusbarmessagelabel.h \
#		generalsettingspage.h bookmarkssettingspage.h editbookmarkdialog.h \
#		settingspagebase.h statusbarspaceinfo.h renamedialog.h sidebar.h sidebarsettings.h sidebarpage.h \
#		bookmarkssidebarpage.h infosidebarpage.h protocolcombo.h
#		infosidebarpage.h filterbar.h protocolcombo.h
#
## let automoc handle all of the meta source files (moc)
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/dolphin.pot
#
## this is where the kdelnk file will go
#kdelnkdir   = $(kde_appsdir)/System
#xdg_apps_DATA = dolphin.desktop
#
## this is where the XML-GUI resource file goes
#rcdir = $(kde_datadir)/dolphin
#rc_DATA = dolphinui.rc
#
#kde_kcfg_DATA = directoryviewpropertysettings.kcfg generalsettings.kcfg \
#				iconsmodesettings.kcfg detailsmodesettings.kcfg previewsmodesettings.kcfg \
#				sidebarsettings.kcfg
#
#KDE_ICON = AUTO
