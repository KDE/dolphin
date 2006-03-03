kde4_header()

add_subdirectory( kwrite ) 
add_subdirectory( kdialog ) 
#add_subdirectory( keditbookmarks ) 

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: Readd keditbookmark )" )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

kde4_footer()
