
add_subdirectory( ftp ) 
add_subdirectory( web ) 



########### install files ###############

install( FILES .directory  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/remote )




#original Makefile.am contents follow:

#SUBDIRS = ftp web
#
#konq_sidebartree_virtuals_remote_data_DATA = .directory
#konq_sidebartree_virtuals_remote_datadir = $(kde_datadir)/konqsidebartng/virtual_folders/remote
