INCLUDES = -I$(top_srcdir)/libkonq $(all_includes) 
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_konq.la

libkcm_konq_la_SOURCES = rootopts.cpp main.cpp behaviour.cpp fontopts.cpp \
			miscopts.cpp

libkcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version
libkcm_konq_la_LIBADD = -lkdeui  

libkcm_konq_la_DATA = overlapping.png onlyone.png
libkcm_konq_ladir = $(kde_datadir)/kcontrol/pics

EXTRA_DIST = $(libkcm_konq_la_DATA)

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(libkcm_konq_la_SOURCES) && mv messages.po $(podir)/kcmkonq.pot


check_PROGRAMS = test
test_SOURCES = $(libkcm_konq_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = -lkdeui                                                    
