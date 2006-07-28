include_directories(  ${CMAKE_SOURCE_DIR}/konqueror/sidebar/ ${CMAKE_SOURCE_DIR}/libkonq  )


########### next target ###############

set(konqsidebar_web_PART_SRCS web_module.cpp )

kde4_automoc(${konqsidebar_web_PART_SRCS})

kde4_add_plugin(konqsidebar_web ${konqsidebar_web_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_web )

target_link_libraries(konqsidebar_web  ${KDE4_KDECORE_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KPARTS_LIBS} konqsidebarplugin konq )

install(TARGETS konqsidebar_web  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES websidebar.html  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/websidebar )
install( FILES webmodule_add.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/add )




#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../ -I$(srcdir)/../../../libkonq $(all_includes)
#
#kde_module_LTLIBRARIES = konqsidebar_web.la
#
#METASOURCES = AUTO
#
#konqsidebar_web_la_SOURCES = web_module.cpp
#konqsidebar_web_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#konqsidebar_web_la_LIBADD = $(LIB_KPARTS) $(LIB_KHTML) ../libkonqsidebarplugin.la ../../../libkonq/libkonq.la
#
##plugindir=$(kde_datadir)/konqsidebartng/entries
##plugin_DATA=websidebar.desktop
#
#pluginadddir=$(kde_datadir)/konqsidebartng/add
#pluginadd_DATA=webmodule_add.desktop
#
#htmldir=$(kde_datadir)/konqsidebartng/websidebar
#html_DATA=websidebar.html
#
