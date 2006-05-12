kde4_header()



########### next target ###############

set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

kde4_automoc(${khtmlkttsdplugin_PART_SRCS})

kde4_add_plugin(khtmlkttsdplugin WITH_PREFIX ${khtmlkttsdplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} khtmlkttsdplugin )

target_link_libraries(khtmlkttsdplugin  ${KDE4_KPARTS_LIBS} khtml )

install_targets(${PLUGIN_INSTALL_DIR} khtmlkttsdplugin )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/khtml/kpartplugins FILES khtmlkttsd.rc khtmlkttsd.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = libkhtmlkttsdplugin.la
#
#libkhtmlkttsdplugin_la_SOURCES = khtmlkttsd.cpp
#libkhtmlkttsdplugin_la_LIBADD  = $(LIB_KHTML)
#libkhtmlkttsdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir = $(kde_datadir)/khtml/kpartplugins
#plugins_DATA = khtmlkttsd.rc khtmlkttsd.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/khtmlkttsd.pot
