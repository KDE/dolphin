INCLUDES = $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_kio.la

libkcm_kio_la_SOURCES = kcookiesdlg.cpp main.cpp ksmboptdlg.cpp \
			useragentdlg.cpp kproxydlg.cpp

libkcm_kio_la_LDFLAGS  = $(all_libraries) -module -avoid-version
libkcm_kio_la_LIBADD = -lkdeui

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(libkcm_kio_la_SOURCES) && mv messages.po $(podir)/kcmkio.pot

check_PROGRAMS = test
test_SOURCES = $(libkcm_kio_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = -lkdeui

