kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/konqueror )


########### next target ###############

set(konq_listview_PART_SRCS 
   konq_listview.cc 
   konq_listviewwidget.cc 
   konq_listviewitems.cc 
   konq_treeviewwidget.cc 
   konq_treeviewitem.cc 
   konq_textviewwidget.cc 
   konq_textviewitem.cc 
   konq_infolistviewwidget.cc 
   konq_infolistviewitem.cc )

kde4_automoc(${konq_listview_PART_SRCS})

kde4_add_kcfg_files(konq_listview_PART_SRCS konq_listviewsettings.kcfgc )

kde4_add_plugin(konq_listview ${konq_listview_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_listview )

target_link_libraries(konq_listview  ${KDE4_KDECORE_LIBS} konq )

install_targets(${PLUGIN_INSTALL_DIR} konq_listview )


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES konq_listview.kcfg )
install_files( ${DATA_INSTALL_DIR}/konqlistview FILES konq_treeview.rc konq_detailedlistview.rc konq_textview.rc            konq_infolistview.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES konq_treeview.desktop konq_detailedlistview.desktop                      konq_textview.desktop konq_infolistview.desktop )

kde4_footer()



#original Makefile.am contents follow:

#
#INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)
#
#kde_module_LTLIBRARIES = konq_listview.la
#
#METASOURCES = AUTO
#
#konq_listview_la_SOURCES = konq_listview.cc \
#                          konq_listviewwidget.cc konq_listviewitems.cc \
#                          konq_treeviewwidget.cc konq_treeviewitem.cc \
#                          konq_textviewwidget.cc konq_textviewitem.cc \
#                          konq_infolistviewwidget.cc konq_infolistviewitem.cc \
#                          konq_listviewsettings.kcfgc 
#                             
#konq_listview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#konq_listview_la_LIBADD = $(top_builddir)/libkonq/libkonq.la
#
#kde_services_DATA = konq_treeview.desktop konq_detailedlistview.desktop \
#                    konq_textview.desktop konq_infolistview.desktop
#
#rcdir = $(kde_datadir)/konqlistview
#rc_DATA = konq_treeview.rc konq_detailedlistview.rc konq_textview.rc \
#          konq_infolistview.rc
#
#kde_kcfg_DATA = konq_listview.kcfg
#
######## Build rules
#konq_listview.lo: konq_listviewsettings.h
#konq_listviewsettings.lo: konq_listviewsettings.h
#konq_listviewwidget.lo: konq_listviewsettings.h
