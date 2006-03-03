kde4_header()

add_subdirectory( ebrowsing ) 
#add_subdirectory( konq ) 
add_subdirectory( konqhtml ) 
add_subdirectory( performance ) 
add_subdirectory( kio ) 
add_subdirectory( filetypes ) 
add_subdirectory( pics ) 
add_subdirectory( css ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )

MESSAGE(STATUS "readd konq into ${CMAKE_CURRENT_SOURCE_DIR}")
########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = ebrowsing konq konqhtml performance kio filetypes pics css
