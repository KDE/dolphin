INCLUDES = $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_kio.la

libkcm_kio_la_SOURCES = kcookiesmain.cpp kcookiespolicies.cpp kcookiesmanagement.cpp \
			main.cpp ksmboptdlg.cpp useragentdlg.cpp kproxydlg.cpp

libkcm_kio_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_kio_la_LIBADD = -lkdeui

messages:
	$(XGETTEXT) $(libkcm_kio_la_SOURCES) -o $(podir)/kcmkio.pot

check_PROGRAMS = test
test_SOURCES = $(libkcm_kio_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = -lkdeui

data_DATA = cookies.desktop proxy.desktop useragent.desktop 
datadir = $(kde_appsdir)/Settings/Browsing/Web
