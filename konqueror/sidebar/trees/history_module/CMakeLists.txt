include_directories( 
	${CMAKE_SOURCE_DIR}/libkonq 
	${CMAKE_SOURCE_DIR}/konqueror/sidebar/
 ${CMAKE_SOURCE_DIR}/konqueror/sidebar/trees/
	 )


########### next target ###############

set(konq_sidebartree_history_PART_SRCS history_module.cpp history_item.cpp history_settings.cpp ${libkonq_sidebar_tree_SRCS} )

kde4_automoc(${konq_sidebartree_history_PART_SRCS})

kde4_add_dcop_skels(konq_sidebartree_history_PART_SRCS history_settings.h )

kde4_add_plugin(konq_sidebartree_history ${konq_sidebartree_history_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartree_history )

target_link_libraries(konq_sidebartree_history  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq )

install_targets(${PLUGIN_INSTALL_DIR} konq_sidebartree_history )


########### next target ###############

set(kcm_history_PART_SRCS kcmhistory.cpp history_settings.cpp )

kde4_automoc(${kcm_history_PART_SRCS})

kde4_add_ui3_files(kcm_history_PART_SRCS history_dlg.ui )

kde4_add_dcop_skels(kcm_history_PART_SRCS history_settings.h )

kde4_add_plugin(kcm_history ${kcm_history_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_history )

target_link_libraries(kcm_history  ${KDE4_KDEUI_LIBS} konq )

install_targets(${PLUGIN_INSTALL_DIR} kcm_history )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kcmhistory.desktop )




#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../..  $(all_includes)
#
#kde_module_LTLIBRARIES = konq_sidebartree_history.la kcm_history.la
#
#METASOURCES = AUTO
#
#konq_sidebartree_history_la_SOURCES = history_module.cpp history_item.cpp \
#	history_settings.cpp history_settings.skel
#konq_sidebartree_history_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#konq_sidebartree_history_la_LIBADD =  ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la
#
#kcm_history_la_SOURCES = kcmhistory.cpp history_dlg.ui history_settings.cpp history_settings.skel
#kcm_history_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#kcm_history_la_LIBADD = -lkdeui $(top_builddir)/libkonq/libkonq.la
#
#xdg_apps_DATA = kcmhistory.desktop
