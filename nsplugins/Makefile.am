SUBDIRS = viewer test control
INCLUDES = -I$(top_srcdir)/libltdl $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = nspluginloader.h kqeventutil.h kxeventutil.h

NSPluginClassIface_DIR = $(srcdir)/viewer

lib_LTLIBRARIES = libnsplugin.la
libnsplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp		\
			NSPluginClassIface.stub				\
			NSPluginCallbackIface.skel

libnsplugin_la_LDFLAGS = $(all_libraries) -version-info 1:1:0 -module -no-undefined
libnsplugin_la_LIBADD  = -lkparts

bin_PROGRAMS = nspluginscan
nspluginscan_SOURCES = pluginscan.cpp
nspluginscan_LDFLAGS =  $(KDE_RPATH) $(all_libraries) -export-dynamic
nspluginscan_LDADD = $(MOTIF_LIBS) $(LIB_KDEUI) $(LIB_KSYCOCA) -lXt

messages:
	$(EXTRACTRC) `find . -name *.ui` > rc.cpp
	$(EXTRACTRC) `find . -name *.rc` >> rc.cpp
	$(XGETTEXT) *.cpp control/*.cpp viewer/*.cpp -o $(podir)/nsplugin.pot

