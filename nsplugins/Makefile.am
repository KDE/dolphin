SUBDIRS = viewer wrapper

INCLUDES = $(all_includes) -I$(top_srcdir)/libltdl

noinst_LIBRARIES = libnsplugin.a

libnsplugin_a_SOURCES = nspluginloader.cpp NSPluginClassIface.stub

METASOURCES = AUTO

check_PROGRAMS = test
test_SOURCES = main.cpp
test_LDFLAGS = $(all_libraries)
test_LDADD = $(LIB_KDEUI) libnsplugin.a -lXt

NSPluginClassIface.h:
	ln -s viewer/NSPluginClassIface.h .


lib_LTLIBRARIES = libplugin.la
libplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp NSPluginClassIface.stub

libplugin_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined
libplugin_la_LIBADD  = -lkparts -lXt


bin_PROGRAMS = pluginscan
pluginscan_SOURCES = pluginscan.cpp
pluginscan_LDFLAGS = $(all_libraries)
pluginscan_LDADD = $(LIB_KDEUI) -lXt
