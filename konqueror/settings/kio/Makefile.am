SUBDIRS = uasproviders

INCLUDES = $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_kio.la

kcm_kio_la_SOURCES = policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp \
      main.cpp smbrodlg.cpp useragentdlg.cpp kproxydlgbase.cpp \
      kproxyexceptiondlg.cpp kmanualproxydlg.cpp kenvvarproxydlg.cpp \
      kproxydlg.cpp kcookiesmanagement.cpp fakeuaprovider.cpp \
      uagentproviderdlg.cpp netpref.cpp cache.cpp ksaveioconfig.cpp

kcm_kio_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_kio_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KIO)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmkio.pot

servicetypes_DATA = uasprovider.desktop
servicetypesdir = $(kde_servicetypesdir)

data_DATA = cookies.desktop useragent.desktop cache.desktop
datadir = $(kde_appsdir)/Settings/WebBrowsing

smb_DATA = smb.desktop netpref.desktop proxy.desktop 
smbdir = $(kde_appsdir)/Settings/Network

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing/smb.desktop
