
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqtextview.la

METASOURCES = AUTO

libkonqtextview_la_SOURCES = konq_textview.cpp konq_textviewitems.cpp konq_textviewwidget.cpp
libkonqtextview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqtextview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_textview.desktop

rcdir = $(kde_datadir)/konqueror
rc_DATA = konq_textview.rc

