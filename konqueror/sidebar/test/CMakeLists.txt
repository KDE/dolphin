


########### next target ###############

set(konq_sidebartest_PART_SRCS konqsidebartest.cpp )

kde4_automoc(${konq_sidebartest_PART_SRCS})

kde4_add_plugin(konq_sidebartest ${konq_sidebartest_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konq_sidebartest )

target_link_libraries(konq_sidebartest  ${KDE4_KDECORE_LIBS} kparts ${KDE4_KIO_LIBS} konqsidebarplugin )

install(TARGETS konq_sidebartest  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES test.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries )




#original Makefile.am contents follow:

## this has all of the subdirectories that make will recurse into.  if
## there are none, comment this out
#SUBDIRS = .
#
## set the include path for X, qt and KDE
#INCLUDES = -I$(srcdir)/.. $(all_includes)
#
## these are the headers for your project
#noinst_HEADERS = konqsidebartest.h
#
## let automoc handle all of the meta source files (moc)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = konq_sidebartest.la
#
## the Part's source, library search path, and link libraries
#konq_sidebartest_la_SOURCES = konqsidebartest.cpp
#konq_sidebartest_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#konq_sidebartest_la_LIBADD  = $(LIB_KPARTS) $(LIB_KIO) ../libkonqsidebarplugin.la
#
## this is where the part's XML-GUI resource file goes
#partrcdir   = $(kde_datadir)/konqsidebartng/entries
#partrc_DATA = test.desktop
