# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
SUBDIRS = .

# set the include path for X, qt and KDE
INCLUDES = $(all_includes) -I$(srcdir)/..

# these are the headers for your project
noinst_HEADERS = konqsidebar_classic_wrap.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

kde_module_LTLIBRARIES = libkonqsidebar_classic_wrap.la

# the Part's source, library search path, and link libraries
libkonqsidebar_classic_wrap_la_SOURCES = konqsidebar_classic_wrap.cpp
libkonqsidebar_classic_wrap_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkonqsidebar_classic_wrap_la_LIBADD  = $(LIB_KPARTS) $(LIB_KFILE) ../libkonqsidebarplugin.la

# this is where the part's XML-GUI resource file goes
partrcdir   = $(kde_datadir)/konqsidebartng/entries
partrc_DATA = sidebar_classic.desktop

partadddir   = $(kde_datadir)/konqsidebartng/add
partadd_DATA = sidebar_classic_add.desktop

