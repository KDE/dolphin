INCLUDES = -I$(top_srcdir)/nsplugins -I$(top_builddir)/nsplugins $(all_includes)
METASOURCES = AUTO

bin_PROGRAMS = nspluginviewer 

nspluginviewer_SOURCES = NSPluginCallbackIface.stub NSPluginClassIface.skel \
	nsplugin.cpp viewer.cpp kxt.cpp 
# qxteventloop.cpp
nspluginviewer_LDFLAGS = $(all_libraries) $(KDE_RPATH) -export-dynamic
nspluginviewer_LDADD = $(LIB_KIO) $(LIB_KPARTS) -lXt

NSPluginCallbackIface_DIR = $(srcdir)/..

