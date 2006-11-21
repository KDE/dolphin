
add_subdirectory( 128x128 )
add_subdirectory( 64x64 )
add_subdirectory( 48x48 )
add_subdirectory( 32x32 )
add_subdirectory( 22x22 )
add_subdirectory( 16x16 )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = 128x128 64x64 48x48 32x32 22x22 16x16
