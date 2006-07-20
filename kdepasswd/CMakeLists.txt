
project(kdepasswd)

add_subdirectory( kcm ) 

########### next target ###############

set(kdepasswd_SRCS kdepasswd.cpp passwd.cpp passwddlg.cpp )

kde4_automoc(${kdepasswd_SRCS})

kde4_add_executable(kdepasswd ${kdepasswd_SRCS})

target_link_libraries(kdepasswd  ${KDE4_KIO_LIBS} )

install(TARGETS kdepasswd DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES kdepasswd.desktop  DESTINATION  ${XDG_APPS_DIR} )




#original Makefile.am contents follow:

#
#SUBDIRS = kcm
#
#bin_PROGRAMS = kdepasswd
#kdepasswd_SOURCES = kdepasswd.cpp passwd.cpp passwddlg.cpp
#kdepasswd_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kdepasswd_LDADD = $(LIB_KIO)
#
#METASOURCES =  AUTO
#AM_CPPFLAGS= -I$(top_srcdir)/libkonq $(all_includes)
#noinst_HEADERS = passwd.h passwddlg.h
#
#xdg_apps_DATA = kdepasswd.desktop
#
#messages:
#	$(XGETTEXT) *.cpp  -o $(podir)/kdepasswd.pot
#
