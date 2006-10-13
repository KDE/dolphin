include_directories( ${CMAKE_SOURCE_DIR}/libltdl ${CMAKE_CURRENT_BINARY_DIR}/..  )


########### next target ###############

if(KDE4_BUILD_TESTS)
set(testnsplugin_SRCS testnsplugin.cpp )

kde4_automoc(testnsplugin ${testnsplugin_SRCS})

kde4_add_executable(testnsplugin ${testnsplugin_SRCS})

target_link_libraries(testnsplugin  ${KDE4_KDEUI_LIBS} nsplugin )

endif(KDE4_BUILD_TESTS)

########### install files ###############





#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libltdl -I.. $(all_includes)
#METASOURCES = AUTO
#noinst_HEADERS = testnsplugin.h
#
#check_PROGRAMS = testnsplugin
#testnsplugin_SOURCES = testnsplugin.cpp
#testnsplugin_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#testnsplugin_LDADD = $(LIB_KDEUI) ../libnsplugin.la
#
#$(srcdir)/testnsplugin.cpp: ../NSPluginClassIface_stub.h
#
