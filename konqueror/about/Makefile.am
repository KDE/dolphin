lib_LTLIBRARIES = libkonqaboutpage.la

INCLUDES = -I.. $(all_includes)
libkonqaboutpage_la_SOURCES = konq_aboutpage.cc
libkonqaboutpage_la_LDFLAGS = -I.. $(all_libraries) $(KDE_PLUGIN)
libkonqaboutpage_la_LIBADD = ../konqueror.la $(LIB_KHTML)

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konq_aboutpage.desktop

METASOURCES = AUTO

datadir = $(kde_datadir)/konqueror/about
data_DATA = aboutkonq.html back.png background.png bgtable.png gohome.png \
	intro.html konq.css konqueror.png lines.png pointers.png shadow1.png \
	specs.html tips.html
