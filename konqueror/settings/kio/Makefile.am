SUBDIRS = uasproviders

AM_CPPFLAGS = $(all_includes)
AM_LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_kio.la

kcm_kio_la_SOURCES = policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp \
      main.cpp smbrodlg.cpp useragentdlg.cpp kproxydlgbase.cpp \
      kmanualproxydlg.cpp kenvvarproxydlg.cpp kproxydlg.cpp \
      kcookiesmanagement.cpp fakeuaprovider.cpp uagentproviderdlg.cpp \
      netpref.cpp cache.cpp ksaveioconfig.cpp socks.cpp socksbase.ui \
      kproxydlg_ui.ui kcookiesmanagementdlg_ui.ui kcookiespoliciesdlg_ui.ui \
      policydlg_ui.ui useragentdlg_ui.ui uagentproviderdlg_ui.ui \
      manualproxy_ui.ui envvarproxy_ui.ui cache_ui.ui

kcm_kio_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
kcm_kio_la_LIBADD = $(LIB_KIO) $(LIB_KUTILS)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmkio.pot

update_DATA = socks.upd
updatedir = $(kde_datadir)/kconf_update

servicetypes_DATA = uasprovider.desktop
servicetypesdir = $(kde_servicetypesdir)

xdg_apps_DATA = cookies.desktop useragent.desktop cache.desktop \
                lanbrowser.desktop netpref.desktop proxy.desktop

smb_DATA = smb.desktop
smbdir = $(kde_appsdir)/.hidden

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/WebBrowsing/smb.desktop
