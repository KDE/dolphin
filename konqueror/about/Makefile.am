kde_module_LTLIBRARIES = konq_aboutpage.la

INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/libkonq $(all_includes)
konq_aboutpage_la_SOURCES = konq_aboutpage.cc
konq_aboutpage_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
konq_aboutpage_la_LIBADD = $(LIB_KHTML)

kde_services_DATA = konq_aboutpage.desktop
kde_servicetypes_DATA = konqaboutpage.desktop

METASOURCES = AUTO

konq_aboutpage_datadir = $(kde_datadir)/konqueror/about
konq_aboutpage_data_DATA = background.png bgtable.png lightning.png\
	intro.html konq.css konqueror.png lines.png pointers.gif shadow1.png \
	specs.html tips.html lines2.png kdelogo2.png more.png \
	konq_rtl.css intro_rtl.html specs_rtl.html tips_rtl.html \
	background_rtl.png bgtable_rtl.png konqueror_rtl.png plugins.html \
	plugins_rtl.html more_rtl.png
