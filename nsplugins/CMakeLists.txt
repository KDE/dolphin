kde4_header()

add_subdirectory( viewer ) 
add_subdirectory( test ) 

include_directories( ${CMAKE_SOURCE_DIR}/libltdl ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(nsplugin_PART_SRCS plugin_part.cpp nspluginloader.cpp )

kde4_automoc(${nsplugin_PART_SRCS})

kde4_add_dcop_skels(nsplugin_PART_SRCS NSPluginCallbackIface.h )

kde4_add_dcop_stubs(nsplugin_PART_SRCS NSPluginClassIface.h )

kde4_add_plugin(nsplugin WITH_PREFIX ${nsplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} nsplugin )

target_link_libraries(nsplugin  ${QT_AND_KDECORE_LIBS} kparts )

install_targets(${LIB_INSTALL_DIR}/kde4 nsplugin )


########### next target ###############

set(kcm_nsplugins_PART_SRCS kcm_nsplugins.cpp )

kde4_automoc(${kcm_nsplugins_PART_SRCS})

kde4_add_plugin(kcm_nsplugins ${kcm_nsplugins_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_nsplugins )

target_link_libraries(kcm_nsplugins  ${QT_AND_KDECORE_LIBS} )

install_targets(${LIB_INSTALL_DIR}/kde4 kcm_nsplugins )


########### next target ###############

set(nspluginscan_SRCS pluginscan.cpp )

kde4_automoc(${nspluginscan_SRCS})

kde4_add_executable(nspluginscan ${nspluginscan_SRCS})

target_link_libraries(nspluginscan  ${QT_AND_KDECORE_LIBS} kdeui Xt )

install_targets(/bin nspluginscan )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES khtml_plugins.desktop )
install_files( ${DATA_INSTALL_DIR}/plugin FILES nspluginpart.rc )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = viewer test
#INCLUDES = -I$(top_srcdir)/libltdl $(all_includes)
#METASOURCES = AUTO
#noinst_HEADERS = nspluginloader.h
#
#NSPluginClassIface_DIR = $(srcdir)/viewer
#
#kde_module_LTLIBRARIES = libnsplugin.la kcm_nsplugins.la
#libnsplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp		\
#			NSPluginClassIface.stub				\
#			NSPluginCallbackIface.skel
#
#libnsplugin_la_LDFLAGS = $(all_libraries) -avoid-version -module $(KDE_PLUGIN) -no-undefined
#libnsplugin_la_LIBADD  = -lkparts
#
#bin_PROGRAMS = nspluginscan
#nspluginscan_SOURCES = pluginscan.cpp
#nspluginscan_LDFLAGS =  $(KDE_RPATH) $(all_libraries) -export-dynamic
#nspluginscan_LDADD = $(LIB_KDEUI) $(LIB_KSYCOCA) -lXt
#
#kcm_nsplugins_la_SOURCES = kcm_nsplugins.cpp
#kcm_nsplugins_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#kcm_nsplugins_la_LIBADD = $(LIB_KDECORE)
#
#xdg_apps_DATA = khtml_plugins.desktop
#
#partrcdir = $(kde_datadir)/plugin
#partrc_DATA = nspluginpart.rc
#
#messages: rc.cpp
##	$(EXTRACTRC) `find . -name "*.ui"` >> rc.cpp
#	$(EXTRACTRC) `find . -name "*.rc"` >> rc.cpp
#	$(XGETTEXT) *.cpp viewer/*.cpp -o $(podir)/nsplugin.pot
#
