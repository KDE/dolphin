kde_module_LTLIBRARIES = libkded_favicons.la

INCLUDES = $(all_includes)
libkded_favicons_la_SOURCES = favicons.cpp favicons.skel
libkded_favicons_la_LDFLAGS = $(all_libraries) -module -avoid-version
libkded_favicons_la_LIBADD = $(LIB_KSYCOCA)

METASOURCES = AUTO

servicesdir = $(kde_servicesdir)/kded
services_DATA = favicons.desktop
