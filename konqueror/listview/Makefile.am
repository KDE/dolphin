
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqlistview.la

METASOURCES = AUTO

libkonqlistview_la_SOURCES = konq_listview.cc konq_listviewitems.cc konq_listviewwidget.cc konq_treeviewwidget.cc konq_textviewwidget.cc konq_textviewitem.cc konq_treeviewitem.cc
libkonqlistview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqlistview_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

servicedir = $(kde_servicesdir)
service_DATA = konq_treeview.desktop konq_detailedlistview.desktop konq_textview.desktop

rcdir = $(kde_datadir)/konqlistview
rc_DATA = konq_treeview.rc konq_detailedlistview.rc konq_textview.rc

