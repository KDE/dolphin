INCLUDES = $(MOTIF_CFLAGS) -I$(top_srcdir)/nsplugins $(all_includes)
METASOURCES = AUTO


bin_PROGRAMS = nspluginviewer

nspluginviewer_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
			nsplugin.cpp viewer.cpp kxt.cpp	

nspluginviewer_LDADD = $(all_libraries) $(MOTIF_LIBS) -lXt $(LIB_KDECORE) $(LIB_KIO) 
nspluginviewer_LDFLAGS = $(KDE_RPATH) -export-dynamic


NSPluginCallbackIface.h:
	-rm -f ./NSPluginCallbackIface.h
	-ln -s $(srcdir)/../NSPluginCallbackIface.h .
