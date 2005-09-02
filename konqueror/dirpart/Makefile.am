INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

kde_module_LTLIBRARIES = konq_part.la

METASOURCES = AUTO

konq_part_la_SOURCES = konq_part.cc konq_model.cc konq_listview.cc konq_iconview.cc konq_selectionmodel.cc konq_itemdelegate.cc konq_iconviewitemdelegate.cc
konq_part_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
konq_part_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

kde_services_DATA = konq_listview.desktop konq_iconview4.desktop

rcdir = $(kde_datadir)/konqpart
rc_DATA = konq_listview.rc konq_iconview.rc
