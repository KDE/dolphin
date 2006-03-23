kde4_header()

add_subdirectory( ikws ) 
add_subdirectory( shorturi ) 
add_subdirectory( localdomain ) 
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#
#SUBDIRS = ikws shorturi localdomain
#
