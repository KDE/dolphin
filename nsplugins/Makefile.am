SUBDIRS = viewer wrapper
INCLUDES = $(all_includes) -I$(top_srcdir)/libltdl
METASOURCES = AUTO

plugin_part.cpp: NSPluginClassIface_stub.h

NSPluginClassIface_stub.h: NSPluginClassIface.h

NSPluginClassIface.h:
	-rm -f ./NSPluginClassIface.h
	-ln -s $(srcdir)/viewer/NSPluginClassIface.h .


lib_LTLIBRARIES = libnsplugin.la
libnsplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp NSPluginClassIface.stub \
			NSPluginCallbackIface.skel

libnsplugin_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined
libnsplugin_la_LIBADD  = -lkparts


bin_PROGRAMS = nspluginscan
nspluginscan_SOURCES = pluginscan.cpp
nspluginscan_LDFLAGS = $(all_libraries)
nspluginscan_LDADD = $(LIB_KDEUI) -lXt

check_PROGRAMS = test
test_SOURCES = main.cpp
test_LDFLAGS = $(all_libraries)
test_LDADD = $(LIB_KDEUI) -lnsplugin
