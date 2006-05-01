#Global section
METASOURCES = AUTO
EXTRA_DIST = DESCRIPTION
SUBDIRS = pics
KDE_ICON = kfind
# set the include path for X, qt and KDE
INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)

noinst_LTLIBRARIES = libkfind_common.la
libkfind_common_la_SOURCES = kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp

#This section is kfindpart
kde_module_LTLIBRARIES = libkfindpart.la

libkfindpart_la_SOURCES = kfindpart.cpp
libkfindpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkfindpart_la_LIBADD = libkfind_common.la $(LIB_KPARTS) $(top_builddir)/libkonq/libkonq.la -lkde3support

noinst_HEADERS	= kftabdlg.h kfind.h version.h kquery.h  kdatecombo.h

xdg_apps_DATA = Kfind.desktop

kde_services_DATA = kfindpart.desktop
kde_servicetypes_DATA = findpart.desktop


#And this is for kfind

bin_PROGRAMS = kfind
kfind_SOURCES = kfwin.cpp kfinddlg.cpp main.cpp 
kfind_LDADD   =  libkfind_common.la $(LIB_KPARTS) -lkde3support

# the library search path.
kfind_LDFLAGS = $(all_libraries) $(KDE_RPATH)
