# this 10 paths are KDE specific. Usage:

# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

doc_DATA = index-1.html index-2.html index-3.html \
	index-4.html index-5.html index.html logotp3.png

docdir = $(kde_htmldir)/en/konsole

EXTRA_DIST = $(doc_DATA) index.nif index.sgml 

doc: index.sgml index.nif
	ksgml2html index en

