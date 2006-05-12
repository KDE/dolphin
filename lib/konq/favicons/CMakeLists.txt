kde4_header()



########### next target ###############

set(kded_favicons_PART_SRCS favicons.cpp )

kde4_automoc(${kded_favicons_PART_SRCS})

set( kded_favicons_DCOP_SKEL_SRCS favicons.h )

kde4_add_dcop_skels(kded_favicons_PART_SRCS ${kded_favicons_DCOP_SKEL_SRCS} )

kde4_add_plugin(kded_favicons ${kded_favicons_PART_SRCS})

target_link_libraries(kded_favicons  ${KDE4_KIO_LIBS}  )

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_favicons )

install_targets(${PLUGIN_INSTALL_DIR} kded_favicons )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES favicons.desktop )
install_files( ${KCONF_UPDATE_INSTALL_DIR} FILES favicons.upd move_favicons.sh )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_favicons.la
#
#INCLUDES = $(all_includes)
#kded_favicons_la_SOURCES = favicons.cpp favicons.skel
#kded_favicons_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_favicons_la_LIBADD = $(LIB_KSYCOCA)
#
#METASOURCES = AUTO
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = favicons.desktop
#
#update_DATA = favicons.upd
#update_SCRIPTS = move_favicons.sh
#updatedir = $(kde_datadir)/kconf_update
