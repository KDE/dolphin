INCLUDES = -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_konq.la

libkcm_konq_la_SOURCES = behaviour.cpp rootopts.cpp main.cpp \
			miscopts.cpp trashopts.cpp fontopts.cpp

libkcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI)

libkcm_konq_la_DATA = overlapping.png onlyone.png
libkcm_konq_ladir = $(kde_datadir)/kcontrol/pics

data_DATA = kcmkonq.desktop
datadir = $(kde_appsdir)/Settings/Browsing/Files

EXTRA_DIST = $(libkcm_konq_la_DATA) $(data_DATA)

messages:
	$(XGETTEXT) $(libkcm_konq_la_SOURCES) -o $(podir)/kcmkonq.pot


check_PROGRAMS = test
test_SOURCES = $(libkcm_konq_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = -lkdeui
