#Global section
METASOURCES = AUTO
EXTRA_DIST = DESCRIPTION
SUBDIRS = pics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

#This section is kfindpart
kde_module_LTLIBRARIES = libkfindpart.la

libkfindpart_la_SOURCES = kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp kfindpart.cpp
libkfindpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkfindpart_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/libkonq/libkonq.la -lkregexpeditor

noinst_HEADERS	= kftabdlg.h kfind.h version.h kquery.h  kdatecombo.h

kde_services_DATA = kfindpart.desktop
kde_servicetypes_DATA = findpart.desktop

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/kfindpart.pot

#And this is for kfind

bin_PROGRAMS = kfind
kfind_SOURCES = kfwin.cpp kfinddlg.cpp main.cpp
kfind_LDADD   =  libkfindpart.la $(top_builddir)/libkonq/libkonq.la

# the library search path.
kfind_LDFLAGS = $(all_libraries) $(KDE_RPATH)
