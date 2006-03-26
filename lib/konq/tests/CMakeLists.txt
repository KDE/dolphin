kde4_header()
include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### next target ###############

set(konqdragtest_SRCS konqdragtest.cpp )

kde4_automoc(${konqdragtest_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(konqdragtest ${konqdragtest_SRCS})

target_link_libraries(konqdragtest  ${KDE4_KDECORE_LIBS} konq )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkonq $(all_includes)
#AM_LDFLAGS = $(all_libraries)
#check_PROGRAMS = konqdragtest
#konqdragtest_SOURCES = konqdragtest.cpp
#konqdragtest_LDADD = ../libkonq.la
#TESTS = konqdragtest
