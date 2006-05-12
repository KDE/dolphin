kde4_header()



########### next target ###############

set(localdomainurifilter_PART_SRCS localdomainurifilter.cpp )

kde4_automoc(${localdomainurifilter_PART_SRCS})

kde4_add_dcop_skels(localdomainurifilter_PART_SRCS localdomainurifilter.h )

kde4_add_plugin(localdomainurifilter WITH_PREFIX ${localdomainurifilter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} localdomainurifilter )

target_link_libraries(localdomainurifilter  ${KDE4_KIO_LIBS} kdeui DCOP ${QT_QT3SUPPORT_LIBRARY})

install_targets(${PLUGIN_INSTALL_DIR} localdomainurifilter )


########### next target ###############

set(klocaldomainurifilterhelper_SRCS klocaldomainurifilterhelper.c )

kde4_automoc(${klocaldomainurifilterhelper_SRCS})

kde4_add_executable(klocaldomainurifilterhelper ${klocaldomainurifilterhelper_SRCS})

target_link_libraries(klocaldomainurifilterhelper  ${KDE4_KDECORE_LIBS} )

install(TARGETS klocaldomainurifilterhelper DESTINATION bin)


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES localdomainurifilter.desktop )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kcontrol/ebrowsing/plugins/localdomain
#
#AM_CPPFLAGS   =  $(all_includes) -DQT_NO_CAST_ASCII
#AM_LDFLAGS   = $(all_libraries)
#
#kde_module_LTLIBRARIES = liblocaldomainurifilter.la
#
#liblocaldomainurifilter_la_LDFLAGS = -module $(KDE_PLUGIN)
#liblocaldomainurifilter_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KDECORE)
#
#liblocaldomainurifilter_la_SOURCES = localdomainurifilter.cpp localdomainurifilter.skel
#
#service_DATA = localdomainurifilter.desktop
#servicedir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = klocaldomainurifilterhelper
#
#klocaldomainurifilterhelper_SOURCES = klocaldomainurifilterhelper.c
#klocaldomainurifilterhelper_LDADD = $(LIBSOCKET)
