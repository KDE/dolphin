INCLUDES = -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_konq.la

kcm_konq_la_SOURCES = behaviour.cpp rootopts.cpp \
		fontopts.cpp previews.cpp \
		desktop.cpp uiserver.stub main.cpp

kcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KSYCOCA)

kcm_konq_la_DATA = overlapping.png onlyone.png
kcm_konq_ladir = $(kde_datadir)/kcontrol/pics

data_DATA = filebehavior.desktop fileappearance.desktop filepreviews.desktop
datadir = $(kde_appsdir)/Settings/FileBrowsing


datadesk_DATA = desktopappearance.desktop desktopbehavior.desktop desktop.desktop
datadeskdir = $(kde_appsdir)/Settings/Desktop

data2_DATA = desktoppath.desktop
data2dir = $(kde_appsdir)/Settings/Personalization

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Settings/LookNFeel/Desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/LookNFeel/Desktop/virtualdesktops.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/LookNFeel/Desktop/desktop.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/LookNFeel/Desktop/borders.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/LookNFeel/Desktop/background.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Settings/Desktop/filetrash.desktop

uiserver_DIR = $(kde_includes)/kio

EXTRA_DIST = $(kcm_konq_la_DATA) $(data_DATA)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kcmkonq.pot

