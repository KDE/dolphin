kde_module_LTLIBRARIES = konq_aboutpage.la

INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/libkonq $(all_includes)
konq_aboutpage_la_SOURCES = konq_aboutpage.cc
konq_aboutpage_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
konq_aboutpage_la_LIBADD = $(LIB_KHTML)

kde_services_DATA = konq_aboutpage.desktop
kde_servicetypes_DATA = konqaboutpage.desktop

METASOURCES = AUTO

konq_aboutpage_datadir = $(kde_datadir)/konqueror/about
konq_aboutpage_data_DATA = \
	bar-bottom-left.png    bar-top-right.png      box-centre-konqueror.png  more.png \
	bar-bottom-middle.png  bottom-left.png        box-middle-left.png       top-left.png \
	bar-bottom-right.png   bottom-middle.png      box-middle-right.png      top-middle.png \
	bar-middle-left.png    bottom-right.png       box-top-left.png          top-right-konqueror.png \
	bar-middle-right.png   box-bottom-left.png    box-top-middle.png \
	bar-top-left.png       box-bottom-middle.png  box-top-right.png \
	bar-top-middle.png     box-bottom-right.png   lightning.png \
	intro.html specs.html tips.html plugins.html \
	intro_rtl.html specs_rtl.html tips_rtl.html plugins_rtl.html
