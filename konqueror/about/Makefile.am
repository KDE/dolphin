lib_LTLIBRARIES = libkonqaboutpage.la

INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/libkonq $(all_includes)
libkonqaboutpage_la_SOURCES = konq_aboutpage.cc
libkonqaboutpage_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqaboutpage_la_LIBADD = $(LIB_KHTML)

kde_services_DATA = konq_aboutpage.desktop
kde_servicetypes_DATA = konqaboutpage.desktop

METASOURCES = AUTO

datadir = $(kde_datadir)/konqueror/about
data_DATA = back.png background.png viewmag.png bgtable.png gohome.png \
	intro.html konq.css konqueror.png lines.png pointers.png shadow1.png \
	specs.html tips.html
