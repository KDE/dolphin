INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_konqhtml.la

kcm_konqhtml_la_SOURCES = htmlopts.cpp jsopts.cpp \
			     javaopts.cpp pluginopts.cpp appearance.cpp \
			     khttpoptdlg.cpp policydlg.cpp main.cpp nsconfigwidget.ui

kcm_konqhtml_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_konqhtml_la_LIBADD = -lkhtml -lkdeui

data_DATA = khtml_behavior.desktop khtml_java_js.desktop khtml_fonts.desktop khtml_plugins.desktop
datadir = $(kde_appsdir)/Settings/WebBrowsing

EXTRA_DIST = $(data_DATA)

messages:
	$(XGETTEXT) $(kcm_konqhtml_la_SOURCES) -o $(podir)/kcmkonqhtml.pot

