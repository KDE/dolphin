
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqlistview.la

METASOURCES = AUTO

libkonqlistview_la_SOURCES = konq_listview.cc konq_listviewitems.cc konq_listviewwidget.cc
libkonqlistview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined
libkonqlistview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_treeview.desktop konq_detailedlistview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_treeview.rc konq_detailedlistview.rc 

