SUBDIRS = viewer #wrapper

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
libplugin_la_SOURCES = main.cpp plugin_part.cpp

libplugin_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libplugin_la_LIBADD  = $(LIB_KFILE) -lkparts

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = plugin.desktop
