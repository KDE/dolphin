SUBDIRS = uasproviders

INCLUDES = $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = libkcm_kio.la

# kcookiesmanagement.cpp
#libkcm_kio_la_SOURCES = policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp \
#			main.cpp ksmboptdlg.cpp useragentdlg.cpp kproxydlg.cpp kcookiesmanagement.cpp

libkcm_kio_la_SOURCES = policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp \
			main.cpp smbrodlg.cpp useragentdlg.cpp kproxydlg.cpp kcookiesmanagement.cpp \
      fakeuaprovider.cpp uagentproviderdlg.cpp kproxydlgui.ui netpref.cpp

libkcm_kio_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_kio_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KFILE)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmkio.pot

check_PROGRAMS = test
test_SOURCES = $(libkcm_kio_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = -lkdeui -lkio

servicetypes_DATA = uasprovider.desktop
servicetypesdir = $(kde_servicetypesdir)

data_DATA = cookies.desktop proxy.desktop useragent.desktop
datadir = $(kde_appsdir)/Settings/WebBrowsing

smb_DATA = smb.desktop netpref.desktop
smbdir = $(kde_appsdir)/Settings/Network

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(kde_appsdir)/Settings/WebBrowsing
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_appsdir)/Settings/WebBrowsing/smb.desktop
