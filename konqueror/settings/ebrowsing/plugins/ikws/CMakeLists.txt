
add_subdirectory( searchproviders ) 



########### next target ###############

set(kuriikwsfilter_PART_SRCS kuriikwsfiltereng.cpp kuriikwsfilter.cpp searchprovider.cpp )

kde4_automoc(${kuriikwsfilter_PART_SRCS})

kde4_add_dcop_skels(kuriikwsfilter_PART_SRCS kuriikwsfilter.h )

kde4_add_plugin(kuriikwsfilter WITH_PREFIX ${kuriikwsfilter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kuriikwsfilter )

target_link_libraries(kuriikwsfilter  ${KDE4_KDEUI_LIBS} DCOP kio ${QT_QT3SUPPORT_LIBRARY})

install_targets(${PLUGIN_INSTALL_DIR} kuriikwsfilter )


########### next target ###############

set(kurisearchfilter_PART_SRCS 
   kurisearchfilter.cpp 
   kuriikwsfiltereng.cpp 
   ikwsopts.cpp 
   searchproviderdlg.cpp 
   searchprovider.cpp )

kde4_automoc(${kurisearchfilter_PART_SRCS})

kde4_add_ui3_files(kurisearchfilter_PART_SRCS searchproviderdlg_ui.ui ikwsopts_ui.ui )

kde4_add_dcop_skels(kurisearchfilter_PART_SRCS kurisearchfilter.h )

kde4_add_plugin(kurisearchfilter WITH_PREFIX ${kurisearchfilter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kurisearchfilter )

target_link_libraries(kurisearchfilter  ${KDE4_KDEUI_LIBS} DCOP ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY} kio)

install_targets(${PLUGIN_INSTALL_DIR} kurisearchfilter )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES kuriikwsfilter.desktop kurisearchfilter.desktop )
install_files( ${SERVICETYPES_INSTALL_DIR} FILES searchprovider.desktop )
install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES kuriikwsfilter.upd )




#original Makefile.am contents follow:

### Makefile.am of kdebase/kcontrol/ebrowsing/plugins/ikws
#
#SUBDIRS = searchproviders
#
#AM_CPPFLAGS   = $(all_includes)
#
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = libkuriikwsfilter.la libkurisearchfilter.la
#
#libkuriikwsfilter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#libkuriikwsfilter_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI)
#
#libkurisearchfilter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#libkurisearchfilter_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI)
#
#libkuriikwsfilter_la_SOURCES = kuriikwsfiltereng.cpp kuriikwsfilter.skel \
#    kuriikwsfilter.cpp searchprovider.cpp
#
#libkurisearchfilter_la_SOURCES = kurisearchfilter.cpp kurisearchfilter.skel \
#      kuriikwsfiltereng.cpp ikwsopts.cpp searchproviderdlg.cpp searchprovider.cpp \
#      searchproviderdlg_ui.ui ikwsopts_ui.ui
#
#update_DATA = kuriikwsfilter.upd
#updatedir = $(kde_datadir)/kconf_update
#
#service_DATA = kuriikwsfilter.desktop kurisearchfilter.desktop
#servicedir = $(kde_servicesdir)
#
#servicetypes_DATA = searchprovider.desktop
#servicetypesdir = $(kde_servicetypesdir)
