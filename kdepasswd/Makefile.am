## Makefile.am for kdepasswd

INCLUDES= $(all_includes)

## kdepasswd
bin_PROGRAMS = 	kdepasswd
kdepasswd_SOURCES = kdepasswd.cpp passwd.cpp passwddlg.cpp
kdepasswd_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kdepasswd_LDADD   = $(LIB_KDEUI) -lkdesu
kdepasswd_METASOURCES =  AUTO
noinst_HEADERS = passwd.h passwddlg.h

desktopdir = $(kde_appsdir)/Settingsmenu
desktop_DATA = kdepasswd.desktop

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Utilities
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Utilities/kdepasswd.desktop

## Messages
messages:
	$(XGETTEXT) $(kdepasswd_SOURCES) -o $(podir)/kdepasswd.pot

