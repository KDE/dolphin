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
	box-centre-konqueror.png  more.png          top-right-konqueror.png  lightning.png \
	launch.html               intro.html        specs.html               tips.html \
	plugins.html              plugins_rtl.html  konq.css
