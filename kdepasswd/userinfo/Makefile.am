# Userinfo Makefile.am

SUBDIRS = pics

kde_module_LTLIBRARIES = kcm_userinfo.la
kcm_userinfo_la_SOURCES = userinfo.cpp userinfo_chfn.cpp chfnproc.cpp \
				userinfo_chface.cpp
kcm_userinfo_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined
kcm_userinfo_la_LIBADD = $(top_builddir)/kdepasswd/libpassdialogs.la $(LIB_KDEUI) $(LIB_KIO) -lkonq

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/kdepasswd $(all_includes)

# the library search path. 
#userinfo_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against.
#userinfo_LDADD   = $(LIB_KFILE)

# these are the headers for your project that won't be installed
noinst_HEADERS = userinfo.h userinfo_chfn.h chfnproc.h \
	userinfo_chface.h userinfo_constants.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages:
	$(XGETTEXT) $(kcm_userinfo_la_SOURCES) -o $(podir)/userinfo.pot

# this is where the desktop file will go 
xdg_apps_DATA = userinfo.desktop
