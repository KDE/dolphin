include_directories( ${CMAKE_SOURCE_DIR}/libkonq )


########### next target ###############

set(kcm_konq_PART_SRCS
   behaviour.cpp
   desktop.cpp
   rootopts.cpp
   fontopts.cpp
   previews.cpp
   browser.cpp
   konqkcmfactory.cpp
   desktopbehavior_impl.cpp )

kde4_automoc(${kcm_konq_PART_SRCS})

kde4_add_ui3_files(kcm_konq_PART_SRCS desktopbehavior.ui )


kde4_add_plugin(kcm_konq ${kcm_konq_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_konq )

target_link_libraries(kcm_konq  ${KDE4_KDE3SUPPORT_LIBS} kutils )

install(TARGETS kcm_konq  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES desktopbehavior.desktop desktop.desktop desktoppath.desktop
    filebehavior.desktop fileappearance.desktop filepreviews.desktop
    filebrowser.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${DATA_INSTALL_DIR}/kcontrol/pics )




#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_srcdir)/libkonq $(all_includes)
#AM_LDFLAGS =  $(all_libraries)
#
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kcm_konq.la
#
#kcm_konq_la_SOURCES = behaviour.cpp desktop.cpp rootopts.cpp \
#		fontopts.cpp previews.cpp browser.cpp \
#		uiserver.stub main.cpp desktopbehavior_impl.cpp \
#		desktopbehavior.ui
#
#kcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
#kcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KSYCOCA) $(LIB_KUTILS)
#
#kcm_konq_la_DATA = overlapping.png onlyone.png
#kcm_konq_ladir = $(kde_datadir)/kcontrol/pics
#
#kcmkonq_data_DATA = filebehavior.desktop fileappearance.desktop filepreviews.desktop
#kcmkonq_datadir = $(kde_appsdir)/.hidden
#
#xdg_apps_DATA = desktopbehavior.desktop desktop.desktop desktoppath.desktop filebrowser.desktop
#
#uiserver_DIR = $(kde_includes)/kio
#
#EXTRA_DIST = $(kcm_konq_la_DATA) $(kcmkonq_data_DATA)
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmkonq.pot
#
