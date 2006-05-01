SUBDIRS = viewer test
INCLUDES = -I$(top_srcdir)/libltdl $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = nspluginloader.h

NSPluginClassIface_DIR = $(srcdir)/viewer

kde_module_LTLIBRARIES = libnsplugin.la kcm_nsplugins.la
libnsplugin_la_SOURCES = plugin_part.cpp nspluginloader.cpp		\
			NSPluginClassIface.stub				\
			NSPluginCallbackIface.skel

libnsplugin_la_LDFLAGS = $(all_libraries) -avoid-version -module $(KDE_PLUGIN) -no-undefined
libnsplugin_la_LIBADD  = -lkparts

bin_PROGRAMS = nspluginscan
nspluginscan_SOURCES = pluginscan.cpp
nspluginscan_LDFLAGS =  $(KDE_RPATH) $(all_libraries) -export-dynamic
nspluginscan_LDADD = $(LIB_KDEUI) $(LIB_KSYCOCA) -lXt

kcm_nsplugins_la_SOURCES = kcm_nsplugins.cpp
kcm_nsplugins_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_nsplugins_la_LIBADD = $(LIB_KDECORE)

xdg_apps_DATA = khtml_plugins.desktop

partrcdir = $(kde_datadir)/plugin
partrc_DATA = nspluginpart.rc


