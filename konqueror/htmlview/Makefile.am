KDE_CXXFLAGS = $(USE_RTTI)

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqhtmlview.la

METASOURCES = AUTO

libkonqhtmlview_la_SOURCES = konq_htmlview.cc kbrowser.moc.cc
libkonqhtmlview_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqhtmlview_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la  $(LIB_KHTML)

# htmlview needs its own copy of kbrowser.moc.cc, to compile it with rtti
kbrowser.moc.cc: $(kde_includes)/kbrowser.h
	$(MOC) $(kde_includes)/kbrowser.h -o $(top_builddir)/konqueror/htmlview/kbrowser.moc.cc

servicedir = $(kde_servicesdir)
service_DATA = konq_htmlview.desktop

