kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nswrapper_LIB_SRCS wrapper.c )

kde4_automoc(${nswrapper_LIB_SRCS})

kde4_add_library(nswrapper SHARED ${nswrapper_LIB_SRCS})

target_link_libraries(nswrapper  ${KDE4_KDECORE_LIBS} )

set_target_properties(nswrapper PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} nswrapper )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#lib_LTLIBRARIES = libnswrapper.la
#libnswrapper_la_SOURCES = wrapper.c
#INCLUDES = -I$(top_srcdir)/nsplugins
#
