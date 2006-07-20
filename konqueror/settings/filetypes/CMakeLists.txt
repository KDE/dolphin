


########### next target ###############

set(libfiletypes_SRCS filetypesview.cpp filetypedetails.cpp filegroupdetails.cpp kservicelistwidget.cpp typeslistitem.cpp newtypedlg.cpp kserviceselectdlg.cpp )

set(kcm_filetypes_PART_SRCS control.cpp ${libfiletypes_SRCS})

kde4_automoc(${kcm_filetypes_PART_SRCS})

kde4_add_plugin(kcm_filetypes ${kcm_filetypes_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_filetypes )

target_link_libraries(kcm_filetypes  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kcm_filetypes  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(keditfiletype_SRCS keditfiletype.cpp ${libfiletypes_SRCS})

kde4_automoc(${keditfiletype_SRCS})

kde4_add_executable(keditfiletype ${keditfiletype_SRCS})

target_link_libraries(keditfiletype  ${KDE4_KIO_LIBS} )

install(TARGETS keditfiletype DESTINATION ${BIN_INSTALL_DIR})


########### next target ###############


########### install files ###############

install( FILES filetypes.desktop  DESTINATION  ${XDG_APPS_DIR} )




#original Makefile.am contents follow:

#AM_CPPFLAGS = $(all_includes)
#
#kcm_filetypes_la_LIBADD	= libfiletypes.la $(LIB_KIO)
#kcm_filetypes_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kcm_filetypes.la
#
#kcm_filetypes_la_SOURCES = control.cpp
#
#noinst_HEADERS = filetypesview.h filetypedetails.h filegroupdetails.h \
#	kservicelistwidget.h typeslistitem.h newtypedlg.h \
#	kserviceselectdlg.h
#
#bin_PROGRAMS = keditfiletype
#
#noinst_LTLIBRARIES = libfiletypes.la
#libfiletypes_la_SOURCES = filetypesview.cpp filetypedetails.cpp filegroupdetails.cpp \
#		 kservicelistwidget.cpp typeslistitem.cpp newtypedlg.cpp \
#		 kserviceselectdlg.cpp
#
#keditfiletype_SOURCES = keditfiletype.cpp
#keditfiletype_LDADD = libfiletypes.la $(LIB_KIO)
#keditfiletype_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/filetypes.pot
#
#xdg_apps_DATA = filetypes.desktop
