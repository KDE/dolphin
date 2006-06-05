


########### next target ###############

set(kdialog_SRCS 
   kdialog.cpp 
   widgets.cpp 
   klistboxdialog.cpp 
   progressdialog.cpp )

kde4_automoc(${kdialog_SRCS})


kde4_add_executable(kdialog ${kdialog_SRCS})

target_link_libraries(kdialog  ${KDE4_KIO_LIBS} )

install(TARGETS kdialog DESTINATION bin)


########### install files ###############





#original Makefile.am contents follow:

#KDE_CXXFLAGS = -DQT_NO_CAST_ASCII -DQT_NO_ASCII_CAST
#INCLUDES = $(all_includes)
#
#bin_PROGRAMS = kdialog
#
#kdialog_SOURCES = kdialog.cpp widgets.cpp klistboxdialog.cpp progressdialog.cpp progressdialogiface.skel
#kdialog_LDADD = $(LIB_KIO)
#kdialog_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#METASOURCES = AUTO
#
#messages:        
#	$(XGETTEXT) *.cpp -o $(podir)/kdialog.pot 
