#    This file is part of the KDE libraries
#    Copyright (C) 1997 David Faure <faure@kde.org>

#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.

#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.

#    You should have received a copy of the GNU Library General Public License
#    along with this library; see the file COPYING.LIB.  If not, write to
#    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#    Boston, MA 02111-1307, USA.

# For Henrik Stormer :)
KDE_CXXFLAGS = $(USE_RTTI) -DQT_NO_ASCII_CAST

INCLUDES = -I$(top_srcdir)/kcontrol/input $(all_includes)

lib_LTLIBRARIES = libkonq.la
libkonq_la_LDFLAGS = $(all_libraries) -version-info 3:0 -no-undefined
libkonq_la_LIBADD = $(LIB_KPARTS)

libkonq_la_SOURCES = kbookmarkmenu.cc kbookmark.cc knewmenu.cc konqpopupmenu.cc \
	konqdirlister.cc konqfileitem.cc \
	kfileivi.cc konqiconviewwidget.cc konqimagepreviewjob.cc \
	konqsettings.cc konqdrag.cc  \
	konqoperations.cc kbookmarkbar.cc \
        konq_dirpart.cc konq_propsview.cc konq_events.cc konq_bgnddlg.cc \
        kdirnotify.stub konq_undo.cc konq_undo.skel

kdirnotify_DIR = $(kde_includes)

METASOURCES = AUTO

include_HEADERS = kbookmark.h kbookmarkmenu.h knewmenu.h konqpopupmenu.h \
	konqdirlister.h konqfileitem.h \
	kfileivi.h konqdrag.h konqiconviewwidget.h konqimagepreviewjob.h \
	konqdefaults.h konqsettings.h \
	konqoperations.h kbookmarkbar.h \
        konq_dirpart.h konq_propsview.h konq_events.h konq_bgnddlg.h \
	konq_undo.h

messages:
	$(XGETTEXT) *.cc *.h -o $(podir)/libkonq.pot

