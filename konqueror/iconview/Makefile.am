
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

kde_module_LTLIBRARIES = konq_iconview.la

METASOURCES = AUTO

konq_iconview_la_SOURCES = konq_iconview.cc
konq_iconview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
konq_iconview_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

noinst_HEADERS = konq_iconview.h

kde_services_DATA = konq_iconview.desktop konq_multicolumnview.desktop

rcdir = $(kde_datadir)/konqiconview
rc_DATA = konq_iconview.rc konq_multicolumnview.rc
