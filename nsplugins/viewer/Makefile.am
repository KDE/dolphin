INCLUDES = -I$(top_srcdir)/nsplugins $(all_includes)
METASOURCES = AUTO


bin_PROGRAMS = nspluginviewer

nspluginviewer_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
			nsplugin.cpp viewer.cpp kxt.cpp	

nspluginviewer_LDADD = $(all_libraries) $(MOTIF_LIBS) $(LIB_KDECORE) $(LIB_KIO) -lXt
nspluginviewer_LDFLAGS = -Wl,--export-dynamic


NSPluginCallbackIface.h:
	-rm -f ./NSPluginCallbackIface.h
	-ln -s $(srcdir)/../NSPluginCallbackIface.h .
