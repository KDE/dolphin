bin_PROGRAMS = nspluginviewer

nspluginviewer_SOURCES = nsplugin.cpp viewer.cpp kxt.cpp NSPluginClassIface.skel \
			NSPluginCallbackIface.stub

nspluginviewer_LDADD = $(all_libraries) $(MOTIF_LIBS) $(LIB_KDECORE) $(LIB_KIO) -lXt
nspluginviewer_LDFLAGS = -Wl,--export-dynamic

METASOURCES = AUTO

INCLUDES = $(all_includes) -I$(top_srcdir)/libltdl -I$(top_srcdir)/nsplugins

NSPluginCallbackIface.h:
	ln -s ../NSPluginCallbackIface.h .
