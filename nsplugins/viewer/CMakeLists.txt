include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp dbusadaptors.cpp callback_proxy.cpp)

kde4_automoc(${nspluginviewer_SRCS})



kde4_add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer  ${KDE4_KPARTS_LIBS} ${X11_Xt_LIB} )

install(TARGETS nspluginviewer DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############





#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/nsplugins -I$(top_builddir)/nsplugins $(all_includes)
#METASOURCES = AUTO
#
#bin_PROGRAMS = nspluginviewer 
#
#nspluginviewer_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
#	nsplugin.cpp viewer.cpp kxt.cpp 
## qxteventloop.cpp
#nspluginviewer_LDFLAGS = $(all_libraries) $(KDE_RPATH) -export-dynamic
#nspluginviewer_LDADD = $(LIB_KIO) $(LIB_KPARTS) -lXt
#
#NSPluginCallbackIface_DIR = $(srcdir)/..
#
