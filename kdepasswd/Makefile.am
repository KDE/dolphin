
SUBDIRS = kcm

bin_PROGRAMS = kdepasswd
kdepasswd_SOURCES = kdepasswd.cpp passwd.cpp passwddlg.cpp
kdepasswd_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kdepasswd_LDADD = $(LIB_KIO)

METASOURCES =  AUTO
AM_CPPFLAGS= -I$(top_srcdir)/libkonq $(all_includes)
noinst_HEADERS = passwd.h passwddlg.h

xdg_apps_DATA = kdepasswd.desktop

messages:
	$(XGETTEXT) *.cpp  -o $(podir)/kdepasswd.pot

