
add_subdirectory( kwrite )
add_subdirectory( kdialog )
add_subdirectory( keditbookmarks )
if(NOT WIN32)
  add_subdirectory( konsole )
endif(NOT WIN32)
add_subdirectory( kfind )

