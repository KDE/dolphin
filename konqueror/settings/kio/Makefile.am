INCLUDES = $(all_includes)
LDFLAGS =  $(all_libraries) $(KDE_RPATH)
LDADD =    $(LIB_KIO) 

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_kio.la

libkcm_kio_la_SOURCES = kcookiesdlg.cpp main.cpp ksmboptdlg.cpp \
			useragentdlg.cpp kproxydlg.cpp

libkcm_kio_la_LDFLAGS  = $(all_libraries) -module -avoid-version
libkcm_kio_la_LIBADD = -lkdeui

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kcmkio_SOURCES) && mv messages.po ../../po/kcmkio.pot

