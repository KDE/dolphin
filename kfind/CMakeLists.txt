
project(kfind)

add_subdirectory( pics ) 
include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(libkfindcommon_SRCS kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp)
set(kfindpart_PART_SRCS kfindpart.cpp ${libkfindcommon_SRCS})

kde4_automoc(${kfindpart_PART_SRCS})

kde4_add_plugin(kfindpart WITH_PREFIX ${kfindpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kfindpart )

target_link_libraries(kfindpart  ${KDE4_KDE3SUPPORT_LIBS} konq )

install(TARGETS kfindpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kfind_SRCS kfwin.cpp kfinddlg.cpp main.cpp ${libkfindcommon_SRCS})

kde4_automoc(${kfind_SRCS})

kde4_add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kfind DESTINATION bin)


########### install files ###############

install( FILES Kfind.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES findpart.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kfindpart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} hicolor )





#original Makefile.am contents follow:

##Global section
#METASOURCES = AUTO
#EXTRA_DIST = DESCRIPTION
#SUBDIRS = pics
#KDE_ICON = kfind
## set the include path for X, qt and KDE
#INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
#
#noinst_LTLIBRARIES = libkfind_common.la
#libkfind_common_la_SOURCES = kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp
#
##This section is kfindpart
#kde_module_LTLIBRARIES = libkfindpart.la
#
#libkfindpart_la_SOURCES = kfindpart.cpp
#libkfindpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
#libkfindpart_la_LIBADD = libkfind_common.la $(LIB_KPARTS) $(top_builddir)/libkonq/libkonq.la -lkde3support
#
#noinst_HEADERS	= kftabdlg.h kfind.h version.h kquery.h  kdatecombo.h
#
#xdg_apps_DATA = Kfind.desktop
#
#kde_services_DATA = kfindpart.desktop
#kde_servicetypes_DATA = findpart.desktop
#
#messages:
#	$(XGETTEXT) *.cpp *.h -o $(podir)/kfindpart.pot
#
##And this is for kfind
#
#bin_PROGRAMS = kfind
#kfind_SOURCES = kfwin.cpp kfinddlg.cpp main.cpp 
#kfind_LDADD   =  libkfind_common.la $(LIB_KPARTS) -lkde3support
#
## the library search path.
#kfind_LDFLAGS = $(all_libraries) $(KDE_RPATH)
