# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
#SUBDIRS = . sidebar_classic trees

#SUBDIRS = .
SUBDIRS = . trees web_module

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kate/lib $(all_includes)

# these are the headers for your project
noinst_HEADERS = konqsidebar.h sidebar_widget.h konqsidebariface_p.h

include_HEADERS = konqsidebarplugin.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

lib_LTLIBRARIES = libkonqsidebarplugin.la 
kde_module_LTLIBRARIES = konq_sidebar.la

# the Part's source, library search path, and link libraries
konq_sidebar_la_SOURCES = konqsidebar.cpp sidebar_widget.cpp 
konq_sidebar_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
konq_sidebar_la_LIBADD  = $(LIB_KPARTS) $(LIB_KIO) ../../libkonq/libkonq.la ./libkonqsidebarplugin.la $(LIB_KUTILS)

libkonqsidebarplugin_la_SOURCES=konqsidebarplugin.cpp
libkonqsidebarplugin_la_LDFLAGS = -version-info 3:0:2 $(all_libraries) -no-undefined
libkonqsidebarplugin_la_LIBADD = $(LIB_KPARTS)

# this is where the desktop file will go 
partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = konq_sidebartng.desktop

# this is where the part's XML-GUI resource file goes
partrcdir   = $(kde_confdir)
partrc_DATA = konqsidebartng.rc

versiondir = $(kde_datadir)/konqsidebartng/entries/
version_DATA=.version

updatedir = $(kde_datadir)/kconf_update
update_DATA = konqsidebartng.upd
update_SCRIPTS = move_konqsidebartng_entries.sh
