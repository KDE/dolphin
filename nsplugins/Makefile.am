SUBDIRS = viewer test control
INCLUDES = -I$(top_srcdir)/libltdl $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = NSPluginClassIface.h NSPluginCallbackIface.h nspluginloader.h

plugin_part.cpp: NSPluginClassIface_stub.h

NSPluginClassIface_stub.h: NSPluginClassIface.h

NSPluginClassIface.h:
	-rm -f ./NSPluginClassIface.h
	-ln -s $(srcdir)/viewer/NSPluginClassIface.h .


lib_LTLIBRARIES = libnsplugin.la
libnsplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp NSPluginClassIface.stub \
			NSPluginCallbackIface.skel javaembed.cpp
libnsplugin_la_LDFLAGS = $(all_libraries) -version-info 1:1:0 -module -no-undefined
libnsplugin_la_LIBADD  = -lkparts

bin_PROGRAMS = nspluginscan
nspluginscan_SOURCES = pluginscan.cpp
nspluginscan_LDFLAGS =  $(KDE_RPATH) $(all_libraries)
nspluginscan_LDADD = $(MOTIF_LIBS) $(LIB_KDEUI) $(LIB_KSYCOCA) -lXt


messages:
	$(EXTRACTRC) `find . -name *.ui` > rc.cpp
	$(EXTRACTRC) `find . -name *.rc` >> rc.cpp
	$(XGETTEXT) *.cpp control/*.cpp viewer/*.cpp -o $(podir)/nsplugin.pot


