INCLUDES= -I$(top_srcdir)/libkonq $(all_includes)

METASOURCES = AUTO

lib_LTLIBRARIES = keditbookmarks.la

keditbookmarks_la_SOURCES = main.cpp \
	toplevel.cpp commands.cpp gcommand.cc

keditbookmarks_la_LIBADD  = $(top_builddir)/libkonq/libkonq.la
keditbookmarks_la_LDFLAGS = $(all_libraries) -module -avoid-version
keditbookmarks_la_METASOURCES = AUTO

bin_PROGRAMS = keditbookmarks

keditbookmarks_SOURCES = dummy.cpp
keditbookmarks_LDADD = keditbookmarks.la
keditbookmarks_LDFLAGS = $(all_libraries) $(KDE_RPATH)

dummy.cpp:
	echo > dummy.cpp

data_DATA = keditbookmarks.desktop
datadir = $(kde_appsdir)/Internet

rcdir = $(kde_datadir)/keditbookmarks
rc_DATA = keditbookmarksui.rc  

KDE_ICON = AUTO
