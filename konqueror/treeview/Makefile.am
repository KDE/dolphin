
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqtreeview.la

METASOURCES = AUTO

libkonqtreeview_la_SOURCES = konq_treeview.cc
libkonqtreeview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqtreeview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_treeview.desktop

