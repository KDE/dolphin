#    This file is part of the KDE libraries
#    Copyright (C) 1997 David Faure <faure@kde.org>
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; see the file COPYING.LIB.  If not, write to
#    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#    Boston, MA 02111-1307, USA.

INCLUDES = $(all_includes)

lib_LTLIBRARIES = libkonq.la
libkonq_la_LDFLAGS = $(all_libraries) -version-info 3:0 -no-undefined
libkonq_la_LIBADD = $(LIB_KPARTS)

libkonq_la_SOURCES = konq_popupmenu.cc knewmenu.cc \
	kbookmarkmenu.cc kbookmark.cc kbookmarkbar.cc kbookmarkimporter.cc \
	kbookmarkmanager.cc kbookmarkmanager.skel \
	konq_dirlister.cc \
	kfileivi.cc konq_iconviewwidget.cc konq_imagepreviewjob.cc \
	konq_settings.cc konq_drag.cc  \
	konq_operations.cc \
        konq_dirpart.cc konq_propsview.cc konq_events.cc konq_bgnddlg.cc \
        konq_undo.cc konq_undo.skel

kdirnotify_DIR = $(kde_includes)

directory_DATA = directory_bookmarkbar.desktop
directorydir   = $(kde_datadir)/kbookmark

METASOURCES = AUTO

include_HEADERS = konq_popupmenu.h knewmenu.h \
	kbookmark.h kbookmarkmenu.h kbookmarkbar.h kbookmarkimporter.h \
	kbookmarkmanager.h \
	konq_dirlister.h konq_fileitem.h \
	kfileivi.h konq_drag.h konq_iconviewwidget.h konq_imagepreviewjob.h \
	konq_defaults.h konq_settings.h \
	konq_operations.h \
        konq_dirpart.h konq_propsview.h konq_events.h konq_bgnddlg.h \
	konq_undo.h

messages:
	$(XGETTEXT) *.cc *.h -o $(podir)/libkonq.pot

