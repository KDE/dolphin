INCLUDES = -I$(srcdir)/../lib/kparts -I$(srcdir)/../libkonq \
	 $(all_includes)
LDFLAGS  = -L../libkonq $(LIB_KHTML) -lkjava $(all_libraries)

SUBDIRS = plugins

lib_LTLIBRARIES = libkonqueror.la

libkonqueror_la_SOURCES = konq_mainview.cc konq_part.cc konq_shell.cc \
			  konq_run.cc konq_childview.cc konq_viewmgr.cc \
			  konq_frame.cc konq_propsview.cc konq_propsmainview.cc \
			  konq_factory.cc konq_events.cc konq_iconview.cc \
			  konq_searchdia.cc konq_txtview.cc \
			  konq_treeview.cc konq_misc.cc konq_progressproxy.cc \
			  konq_htmlview.cc konq_actions.cc

libkonqueror_la_METASOURCES = AUTO

libkonqueror_la_LDFLAGS = -version-info 1:0:0

libkonqueror_la_LIBADD = -lkparts -lkonq $(LIB_KIO) $(LIB_KDEUI) $(LIB_QT)

bin_PROGRAMS = konqueror

konqueror_SOURCES = konq_main.cc

konqueror_LDFLAGS = $(KDE_RPATH)

konqueror_LDADD = libkonqueror.la -lkded $(LIBMICO)

