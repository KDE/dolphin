include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(konq_aboutpage_PART_SRCS konq_aboutpage.cc )

kde4_automoc(${konq_aboutpage_PART_SRCS})

kde4_add_plugin(konq_aboutpage ${konq_aboutpage_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_aboutpage )

target_link_libraries(konq_aboutpage  ${KDE4_KDECORE_LIBS} khtml kdeui kparts  ${QT_QTGUI_LIBRARY})

install(TARGETS konq_aboutpage  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES box-centre-konqueror.png  top-right-konqueror.png  lightning.png  	launch.html               intro.html        specs.html               tips.html  	plugins.html              plugins_rtl.html  konq.css  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/about )
install( FILES konqaboutpage.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES konq_aboutpage.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = konq_aboutpage.la
#
#INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/libkonq $(all_includes)
#konq_aboutpage_la_SOURCES = konq_aboutpage.cc
#konq_aboutpage_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#konq_aboutpage_la_LIBADD = $(LIB_KHTML)
#
#kde_services_DATA = konq_aboutpage.desktop
#kde_servicetypes_DATA = konqaboutpage.desktop
#
#METASOURCES = AUTO
#
#konq_aboutpage_datadir = $(kde_datadir)/konqueror/about
#konq_aboutpage_data_DATA = \
#	box-centre-konqueror.png  top-right-konqueror.png  lightning.png \
#	launch.html               intro.html        specs.html               tips.html \
#	plugins.html              plugins_rtl.html  konq.css
