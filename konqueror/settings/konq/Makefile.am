INCLUDES = -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = libkcm_konq.la

libkcm_konq_la_SOURCES = behaviour.cpp rootopts.cpp \
		trashopts.cpp fontopts.cpp \
		desktop.cpp uiserver.stub main.cpp

libkcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KSYCOCA)

libkcm_konq_la_DATA = overlapping.png onlyone.png
libkcm_konq_ladir = $(kde_datadir)/kcontrol/pics

data_DATA = kcmkonq.desktop
datadir = $(kde_appsdir)/Settings/FileBrowsing

data2_DATA = desktop.desktop virtualdesktops.desktop
data2dir = $(kde_appsdir)/Settings/LookNFeel

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(kde_appsdir)/Settings/LookNFeel/Desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_appsdir)/Settings/LookNFeel/Desktop/virtualdesktops.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_appsdir)/Settings/LookNFeel/Desktop/desktop.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_appsdir)/Settings/LookNFeel/Desktop/borders.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_appsdir)/Settings/LookNFeel/Desktop/background.desktop

uiserver_DIR = $(kde_includes)/kio

EXTRA_DIST = $(libkcm_konq_la_DATA) $(data_DATA)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kcmkonq.pot

