
INCLUDES= -I$(top_srcdir)/libkonq/ $(all_includes)

bin_PROGRAMS = kfind
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KFILE) $(top_builddir)/libkonq/libkonq.la

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfindtop.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp moc.cpp

METASOURCES  =  kfdird.moc kfwin.moc kfindtop.moc \
             kftabdlg.moc kfoptions.moc kfind.moc

noinst_HEADERS	  =  kfarch.h kfdird.h kfindtop.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h version.h

KDE_ICON = kfind

EXTRA_DIST = DESCRIPTION

SUBDIRS = pics

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kfind_SOURCES) && mv messages.po ../po/kfind.pot

