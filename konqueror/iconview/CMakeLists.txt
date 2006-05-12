include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############

set(konq_iconview_PART_SRCS konq_iconview.cc )

kde4_automoc(${konq_iconview_PART_SRCS})

kde4_add_plugin(konq_iconview ${konq_iconview_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_iconview )

target_link_libraries(konq_iconview  ${KDE4_KDECORE_LIBS} konq )

install_targets(${PLUGIN_INSTALL_DIR} konq_iconview )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqiconview FILES konq_iconview.rc konq_multicolumnview.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES konq_iconview.desktop konq_multicolumnview.desktop )




#original Makefile.am contents follow:

#
#INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)
#
#kde_module_LTLIBRARIES = konq_iconview.la
#
#METASOURCES = AUTO
#
#konq_iconview_la_SOURCES = konq_iconview.cc
#konq_iconview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#konq_iconview_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
#
#noinst_HEADERS = konq_iconview.h
#
#kde_services_DATA = konq_iconview.desktop konq_multicolumnview.desktop
#
#rcdir = $(kde_datadir)/konqiconview
#rc_DATA = konq_iconview.rc konq_multicolumnview.rc
