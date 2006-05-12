include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )

kde4_automoc(${konq_shellcmdplugin_PART_SRCS})

kde4_add_plugin(konq_shellcmdplugin ${konq_shellcmdplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_shellcmdplugin )

target_link_libraries(konq_shellcmdplugin  ${KDE4_KDECORE_LIBS} konq ${KDE4_KDESU_LIBS})

install_targets(${PLUGIN_INSTALL_DIR} konq_shellcmdplugin )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqlistview/kpartplugins FILES kshellcmdplugin.rc kshellcmdplugin.desktop )
install_files( ${DATA_INSTALL_DIR}/konqiconview/kpartplugins FILES kshellcmdplugin.rc kshellcmdplugin.desktop )




#original Makefile.am contents follow:

#INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
#METASOURCES=AUTO
#
#kde_module_LTLIBRARIES = konq_shellcmdplugin.la
#konq_shellcmdplugin_la_SOURCES = kshellcmdexecutor.cpp kshellcmddialog.cpp \
#	kshellcmdplugin.cpp
#konq_shellcmdplugin_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
#konq_shellcmdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#iconviewdir = $(kde_datadir)/konqiconview/kpartplugins
#iconview_DATA = kshellcmdplugin.rc kshellcmdplugin.desktop
#
#listviewdir = $(kde_datadir)/konqlistview/kpartplugins
#listview_DATA = kshellcmdplugin.rc kshellcmdplugin.desktop
