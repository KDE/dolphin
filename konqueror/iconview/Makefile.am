
AM_CPPFLAGS = -DQT_NO_ASCII_CAST

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqiconview.la

METASOURCES = AUTO

libkonqiconview_la_SOURCES = konq_iconview.cc
libkonqiconview_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqiconview_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

servicedir = $(kde_servicesdir)
service_DATA = konq_iconview.desktop konq_multicolumnview.desktop

rcdir = $(kde_datadir)/konqiconview
rc_DATA = konq_iconview.rc konq_multicolumnview.rc
