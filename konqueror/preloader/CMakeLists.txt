include_directories( ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############
set(kded_konqy_preloader_PART_SRCS preloader.cc )

kde4_add_kcfg_files(kded_konqy_preloader_PART_SRCS ${CMAKE_SOURCE_DIR}/konqueror/konq_settingsxt.kcfgc)

qt4_add_dbus_adaptor( kded_konqy_preloader_PART_SRCS org.kde.konqueror.Preloader.xml preloader.h KonqyPreloader )

kde4_automoc(${kded_konqy_preloader_PART_SRCS})


kde4_add_plugin(kded_konqy_preloader ${kded_konqy_preloader_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_konqy_preloader )

target_link_libraries(kded_konqy_preloader  ${KDE4_KDEUI_LIBS} )

install(TARGETS kded_konqy_preloader  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install_files( /share/autostart FILES konqy_preload.desktop )
install( FILES konqy_preloader.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )




#original Makefile.am contents follow:

#AM_CPPFLAGS = -DQT_NO_CAST_ASCII
#
#kde_module_LTLIBRARIES = kded_konqy_preloader.la
#
#INCLUDES= -I.. $(all_includes)
#kded_konqy_preloader_la_SOURCES = preloader.cc preloader.skel
#kded_konqy_preloader_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_konqy_preloader_la_LIBADD = ../libkonqueror_intern.la $(LIB_KSYCOCA)
#
#METASOURCES = AUTO
#
#KonquerorIface_DIR =  $(srcdir)/..
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = konqy_preloader.desktop
#
#autostartdir = $(datadir)/autostart
#autostart_DATA = konqy_preload.desktop
#
######## Build rules
#preloader.lo: ../konq_settingsxt.h
