kde_module_LTLIBRARIES = kded_favicons.la

INCLUDES = $(all_includes)
kded_favicons_la_SOURCES = favicons.cpp favicons.skel
kded_favicons_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_favicons_la_LIBADD = $(LIB_KSYCOCA)

METASOURCES = AUTO

servicesdir = $(kde_servicesdir)/kded
services_DATA = favicons.desktop

update_DATA = favicons.upd
update_SCRIPTS = move_favicons.sh
updatedir = $(kde_datadir)/kconf_update
