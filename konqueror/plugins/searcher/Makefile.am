INCLUDES  = $(MICO_INCLUDES) $(all_includes)
LDFLAGS   = $(all_libraries) $(KDE_RPATH) $(MICO_LDFLAGS)
LDADD = -lkparts -lcutesti
IDL_FLAGS = --relative-paths --any -I$(idldir)

bin_PROGRAMS = konq_searcher

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

rc_DATA = konq_searcherrc
rcdir = $(kde_confdir)

konq_searcher_SOURCES = main.cc configwidget.cc enginecfg.cc

METASOURCES = AUTO