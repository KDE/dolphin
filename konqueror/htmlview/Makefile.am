KDE_CXXFLAGS = $(USE_RTTI)

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqhtmlview.la

METASOURCES = AUTO

libkonqhtmlview_la_SOURCES = konq_htmlview.cc
libkonqhtmlview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqhtmlview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la  $(LIB_KHTML)

servicedir = $(kde_servicesdir)
service_DATA = konq_htmlview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_htmlview.rc

