
project(libkonq)

add_subdirectory( pics )
add_subdirectory( favicons )
add_subdirectory( tests )

include_directories( ${ZLIB_INCLUDE_DIR}  )


########### next target ###############

# Please use .cpp for new files
set(konq_LIB_SRCS
   konq_popupmenu.cc
   knewmenu.cc
   konq_xmlguiclient.cc
   kfileivi.cc
   konq_iconviewwidget.cc
   konq_settings.cc
   konq_operations.cc
   konq_dirpart.cc
   konq_propsview.cc
   konq_events.cc
   konq_bgnddlg.cc
   konq_undo.cc
   konq_undoadaptor.cpp
   konq_historymgr.cc
   konq_historymgr_adaptor.cpp
   konq_historyentry.cpp
   konq_pixmapprovider.cc
   kivdirectoryoverlay.cc
   konq_faviconmgr.cc
   konq_filetip.cc
   konqmimedata.cpp )

kde4_automoc(${konq_LIB_SRCS})

kde4_add_library(konq SHARED ${konq_LIB_SRCS})

target_link_libraries(konq ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} kdefx ${KDE4_KDESU_LIBS})

set_target_properties(konq PROPERTIES VERSION 5.0.0 SOVERSION 5 )
install_targets(${LIB_INSTALL_DIR} konq )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kbookmark FILES directory_bookmarkbar.desktop  )
install_files( /include FILES
 konq_popupmenu.h
 knewmenu.h
 kfileivi.h
 konq_iconviewwidget.h
 konq_defaults.h
 konq_settings.h
 konq_operations.h
 libkonq_export.h
 konq_dirpart.h
 konq_propsview.h
 konq_events.h
 konq_undo.h
 konq_historymgr.h
 konq_historymgr_adaptor.h
 konq_historyentry.h
 konq_pixmapprovider.h
 kivdirectoryoverlay.h
 konq_faviconmgr.h
 konq_xmlguiclient.h
 konqbookmarkmanager.h
 konq_filetip.h
 konqmimedata.h
)
install_files( ${SERVICETYPES_INSTALL_DIR} FILES konqpopupmenuplugin.desktop )




#original Makefile.am contents follow:

##    This file is part of the KDE libraries
##    Copyright (C) 1997 David Faure <faure@kde.org>
##
##    This library is free software; you can redistribute it and/or
##    modify it under the terms of the GNU Library General Public
##    License as published by the Free Software Foundation; either
##    version 2 of the License, or (at your option) any later version.
##
##    This library is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##    Library General Public License for more details.
##
##    You should have received a copy of the GNU Library General Public License
##    along with this library; see the file COPYING.LIB.  If not, write to
##    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
##    Boston, MA 02110-1301, USA.
#
#INCLUDES = -I$(kde_includes)/arts $(all_includes)
#SUBDIRS = pics favicons
#
#lib_LTLIBRARIES = libkonq.la
#libkonq_la_LDFLAGS = $(all_libraries) -version-info 6:0:2 -no-undefined
#libkonq_la_LIBADD = $(LIB_KPARTS) $(LIB_KDE3SUPPORT)
#
#libkonq_la_SOURCES = konq_popupmenu.cc knewmenu.cc \
#   konq_xmlguiclient.cc\
#   kfileivi.cc konq_iconviewwidget.cc konq_settings.cc \
#   konq_operations.cc \
#   konq_dirpart.cc konq_propsview.cc konq_events.cc konq_bgnddlg.cc \
#   konq_undo.cc konq_undo.skel \
#   konq_historymgr.cc konq_historycomm.cc konq_historycomm.skel \
#   konq_pixmapprovider.cc \
#   kivdirectoryoverlay.cc \
#   konq_faviconmgr.cc konq_faviconmgr.skel konq_filetip.cc konqmimedata.cpp
#
#directory_DATA = directory_bookmarkbar.desktop
#directorydir   = $(kde_datadir)/kbookmark
#
#servicetype_DATA = konqpopupmenuplugin.desktop
#servicetypedir = $(kde_servicetypesdir)
#
#METASOURCES = AUTO
#
#include_HEADERS = konq_popupmenu.h knewmenu.h \
#    kfileivi.h konq_iconviewwidget.h \
#    konq_defaults.h konq_settings.h \
#    konq_operations.h libkonq_export.h \
#    konq_dirpart.h konq_propsview.h konq_events.h \
#    konq_undo.h konq_historymgr.h konq_historycomm.h \
#    konq_pixmapprovider.h \
#    kivdirectoryoverlay.h \
#    konq_faviconmgr.h konq_xmlguiclient.h konqbookmarkmanager.h konq_filetip.h konqmimedata.h
#
#
#if include_ARTS
#ARTS_MODULE = konq_sound.la
#endif
#
#kde_module_LTLIBRARIES = $(ARTS_MODULE)
#konq_sound_la_SOURCES = konq_sound.cc
#konq_sound_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#konq_sound_la_LIBADD = -lsoundserver_idl -lartskde
#
#noinst_HEADERS = konq_sound.h
#
#messages:
#	$(XGETTEXT) *.cc *.h -o $(podir)/libkonq.pot
#
#DOXYGEN_REFERENCES = dcop kdecore kio/bookmarks
#include ../admin/Doxyfile.am
#
