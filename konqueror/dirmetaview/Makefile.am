
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqdirmetaview.la

servicedir = $(kde_servicesdir)
service_DATA = konq_dirmetaview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_dirmetaview.rc

METASOURCES = AUTO

libkonqdirmetaview_la_SOURCES = konq_dirmetaview.cc
libkonqdirmetaview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqdirmetaview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

