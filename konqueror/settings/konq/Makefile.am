
INCLUDES = -I$(top_srcdir)/libkonq $(all_includes) 
LDFLAGS =  $(all_libraries) $(KDE_RPATH)
LDADD =    $(LIB_KDEUI) 

#bin_PROGRAMS = kcmkonq
METASOURCES = AUTO
noinst_HEADERS = htmlopts.h khttpoptdlg.h miscopts.h behaviour.h rootopts.h

#kcmkonq_SOURCES = main.cpp htmlopts.cpp khttpoptdlg.cpp miscopts.cpp behaviour.cpp

lib_LTLIBRARIES = libkcm_konq.la

libkcm_konq_la_SOURCES = rootopts.cpp main.cpp behaviour.cpp htmlopts.cpp \
			khttpoptdlg.cpp miscopts.cpp

libkcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version
libkcm_konq_la_LIBADD = -lkdeui  

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kcmkonq_SOURCES) && mv messages.po ../../po/kcmkonq.pot

