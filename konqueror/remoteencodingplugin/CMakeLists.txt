include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(konq_remoteencoding_PART_SRCS kremoteencodingplugin.cpp )

kde4_automoc(${konq_remoteencoding_PART_SRCS})

kde4_add_plugin(konq_remoteencoding ${konq_remoteencoding_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_remoteencoding )

target_link_libraries(konq_remoteencoding  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS konq_remoteencoding  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kremoteencodingplugin.rc kremoteencodingplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqlistview/kpartplugins )
install( FILES kremoteencodingplugin.rc kremoteencodingplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqiconview/kpartplugins )




#original Makefile.am contents follow:

#INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)
#METASOURCES=AUTO
#
#kde_module_LTLIBRARIES = konq_remoteencoding.la
#konq_remoteencoding_la_SOURCES = kremoteencodingplugin.cpp
#konq_remoteencoding_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
#konq_remoteencoding_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#iconviewdir = $(kde_datadir)/konqiconview/kpartplugins
#iconview_DATA = kremoteencodingplugin.rc kremoteencodingplugin.desktop
#
#listviewdir = $(kde_datadir)/konqlistview/kpartplugins
#listview_DATA = kremoteencodingplugin.rc kremoteencodingplugin.desktop
