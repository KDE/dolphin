## Makefile.am for kdepasswd

SUBDIRS = . userinfo
INCLUDES= $(all_includes)

## kdepasswd
## Abstract away the dialogs so that userinfo can use them.
noinst_LTLIBRARIES = libpassdialogs.la
libpassdialogs_la_SOURCES = passwd.cpp passwddlg.cpp
libpassdialogs_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)
bin_PROGRAMS = 	kdepasswd
kdepasswd_SOURCES = kdepasswd.cpp
kdepasswd_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kdepasswd_LDADD   = libpassdialogs.la $(LIB_KDEUI) -lkdesu
METASOURCES =  AUTO
noinst_HEADERS = passwd.h passwddlg.h

xdg_apps_DATA = kdepasswd.desktop

## Messages
messages:
	$(XGETTEXT) $(kdepasswd_SOURCES) $(libpassdialogs_la_SOURCES) -o $(podir)/kdepasswd.pot

