# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
SUBDIRS = . sidebar_classic trees

# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = konqsidebar.h sidebar_widget.h

#include_HEADERS = konqsidebarplugin.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

lib_LTLIBRARIES = libkonqsidebarplugin.la libkonqsidebar.la

# the Part's source, library search path, and link libraries
libkonqsidebar_la_SOURCES = konqsidebar.cpp sidebar_widget.cpp
libkonqsidebar_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkonqsidebar_la_LIBADD  = $(LIB_KPARTS) $(LIB_KFILE) ./libkonqsidebarplugin.la

libkonqsidebarplugin_la_SOURCES=konqsidebarplugin.cpp
libkonqsidebarplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkonqsidebarplugin_la_LIBADD = $(LIB_QT) $(LIB_KDECORE) $(LIB_KPARTS)

# this is where the desktop file will go 
partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = konq_sidebartng.desktop

# this is where the part's XML-GUI resource file goes
partrcdir   = $(kde_confdir)
partrc_DATA = konqsidebartng.rc

versiondir = $(kde_datadir)/konqsidebartng/entries/
version_DATA=.version
