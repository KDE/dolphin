INCLUDES  = $(MICO_INCLUDES) $(all_includes)
LDFLAGS   = $(all_libraries) $(KDE_RPATH) $(MICO_LDFLAGS)
LDADD = -lkparts
IDL_FLAGS = --query-server-for-narrow --relative-paths --any -I$(idldir)

bin_PROGRAMS = konq_searcher

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

konq_searcher_SOURCES = browser.idl main.cc configwidget.cc

main.cc: browser.h

browser_IDLDIR = $(idldir)
