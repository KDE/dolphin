
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqiconview.la

METASOURCES = AUTO

libkonqiconview_la_SOURCES = konq_iconview.cc
libkonqiconview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined
libkonqiconview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_iconview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_iconview.rc
