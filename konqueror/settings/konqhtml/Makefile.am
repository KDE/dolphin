INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = libkcm_konqhtml.la

libkcm_konqhtml_la_SOURCES = htmlopts.cpp jsopts.cpp \
			     javaopts.cpp appearance.cpp \
			     khttpoptdlg.cpp policydlg.cpp main.cpp

libkcm_konqhtml_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_konqhtml_la_LIBADD = -lkhtml -lkdeui

data_DATA = konqhtml.desktop
datadir = $(kde_appsdir)/Settings/WebBrowsing

EXTRA_DIST = $(data_DATA)

messages:
	$(XGETTEXT) $(libkcm_konqhtml_la_SOURCES) -o $(podir)/kcmkonqhtml.pot


#bin_PROGRAMS = test
#test_SOURCES = test.cpp rootopts.cpp main.cpp behaviour.cpp htmlopts.cpp \
#                        khttpoptdlg.cpp miscopts.cpp
#test.cpp:
#	echo "int main() {};" > test.cpp
#test_LDFLAGS  = $(all_libraries) -module -avoid-version
#test_LDADD = -lkdeui
