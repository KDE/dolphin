INCLUDES = $(MOTIF_CFLAGS) -I$(top_srcdir)/nsplugins $(all_includes)
METASOURCES = AUTO

bin_PROGRAMS = nspluginviewer kflash.kss

nspluginviewer_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
	nsplugin.cpp viewer.cpp kxt.cpp	
nspluginviewer_LDADD = $(all_libraries) $(MOTIF_LIBS) $(LIB_KDECORE) \
	$(LIB_KIO) -lXt
nspluginviewer_LDFLAGS = $(KDE_RPATH) -export-dynamic

kflash.kss_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
	nsplugin.cpp kflash.cpp kxt.cpp	
kflash.kss_LDADD = $(all_libraries) $(MOTIF_LIBS) $(LIB_KDECORE) \
	$(LIB_KIO) -lXt
kflash.kss_LDFLAGS = $(KDE_RPATH) -export-dynamic

NSPluginCallbackIface.h:
	-rm -f ./NSPluginCallbackIface.h
	-ln -s $(srcdir)/../NSPluginCallbackIface.h .
