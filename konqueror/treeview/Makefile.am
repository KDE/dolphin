
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqtreeview.la

METASOURCES = AUTO

libkonqtreeview_la_SOURCES = konq_treeview.cc konq_treeviewitems.cc konq_treeviewwidget.cc
libkonqtreeview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined
libkonqtreeview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_treeview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_treeview.rc

