
INCLUDES=-I$(top_srcdir)/libkonq $(all_includes)

kde_module_LTLIBRARIES = libkfindpart.la

####### Files

libkfindpart_la_SOURCES = kftabdlg.cpp kfind.cpp kquery.cpp kfindpart.cpp
libkfindpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkfindpart_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/libkonq/libkonq.la

METASOURCES = AUTO

noinst_HEADERS	= kftabdlg.h kfind.h version.h kquery.h

EXTRA_DIST = DESCRIPTION

SUBDIRS = pics

kde_services_DATA = kfindpart.desktop
kde_servicetypes_DATA = findpart.desktop

 
messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/kfindpart.pot

