
SUBDIRS = kcm

bin_PROGRAMS = kdepasswd
kdepasswd_SOURCES = kdepasswd.cpp passwd.cpp passwddlg.cpp
kdepasswd_LDFLAGS = $(all_libraries)
kdepasswd_LDADD = $(LIB_KIO)

METASOURCES =  AUTO
AM_CPPFLAGS= $(all_includes) -I$(top_srcdir)/libkonq
noinst_HEADERS = passwd.h passwddlg.h

xdg_apps_DATA = kdepasswd.desktop

messages:
	$(XGETTEXT) $(kdepasswd_SOURCES)  -o $(podir)/kdepasswd.pot

