kde4_header()

add_subdirectory( trees ) 
add_subdirectory( web_module ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/kate/lib  )


########### next target ###############

set(konqsidebarplugin_LIB_SRCS konqsidebarplugin.cpp )

kde4_automoc(${konqsidebarplugin_LIB_SRCS})

kde4_add_library(konqsidebarplugin SHARED ${konqsidebarplugin_LIB_SRCS})

target_link_libraries(konqsidebarplugin  ${KDE4_KDECORE_LIBS} kparts )

set_target_properties(konqsidebarplugin PROPERTIES VERSION 1.2.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} konqsidebarplugin )


########### next target ###############

set(konq_sidebar_PART_SRCS konqsidebar.cpp sidebar_widget.cpp )

kde4_automoc(${konq_sidebar_PART_SRCS})

kde4_add_plugin(konq_sidebar ${konq_sidebar_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebar )

target_link_libraries(konq_sidebar  ${KDE4_KDECORE_LIBS} kparts kio konq konqsidebarplugin kutils )

install_targets(${PLUGIN_INSTALL_DIR} konq_sidebar )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES konq_sidebartng.desktop )
install_files( ${CONFIG_INSTALL_DIR} FILES konqsidebartng.rc )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/entries/ FILES .version )
install_files( /include FILES konqsidebarplugin.h )
install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES konqsidebartng.upd move_konqsidebartng_entries.sh )

kde4_footer()



#original Makefile.am contents follow:

## this has all of the subdirectories that make will recurse into.  if
## there are none, comment this out
##SUBDIRS = . sidebar_classic trees
#
##SUBDIRS = .
#SUBDIRS = . trees web_module
#
## set the include path for X, qt and KDE
#INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kate/lib $(all_includes)
#
## these are the headers for your project
#noinst_HEADERS = konqsidebar.h sidebar_widget.h konqsidebariface_p.h
#
#include_HEADERS = konqsidebarplugin.h
#
## let automoc handle all of the meta source files (moc)
#METASOURCES = AUTO
#
#lib_LTLIBRARIES = libkonqsidebarplugin.la 
#kde_module_LTLIBRARIES = konq_sidebar.la
#
## the Part's source, library search path, and link libraries
#konq_sidebar_la_SOURCES = konqsidebar.cpp sidebar_widget.cpp 
#konq_sidebar_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#konq_sidebar_la_LIBADD  = $(LIB_KPARTS) $(LIB_KIO) ../../libkonq/libkonq.la ./libkonqsidebarplugin.la $(LIB_KUTILS)
#
#libkonqsidebarplugin_la_SOURCES=konqsidebarplugin.cpp
#libkonqsidebarplugin_la_LDFLAGS = -version-info 3:0:2 $(all_libraries) -no-undefined
#libkonqsidebarplugin_la_LIBADD = $(LIB_KPARTS)
#
## this is where the desktop file will go 
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = konq_sidebartng.desktop
#
## this is where the part's XML-GUI resource file goes
#partrcdir   = $(kde_confdir)
#partrc_DATA = konqsidebartng.rc
#
#versiondir = $(kde_datadir)/konqsidebartng/entries/
#version_DATA=.version
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = konqsidebartng.upd
#update_SCRIPTS = move_konqsidebartng_entries.sh
