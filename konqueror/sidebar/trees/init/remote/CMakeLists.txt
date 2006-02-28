kde4_header()

add_subdirectory( ftp ) 
add_subdirectory( web ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/remote FILES .directory )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = ftp web
#
#konq_sidebartree_virtuals_remote_data_DATA = .directory
#konq_sidebartree_virtuals_remote_datadir = $(kde_datadir)/konqsidebartng/virtual_folders/remote
