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

INCLUDES = $(all_includes) $(MICO_INCLUDES)

lib_LTLIBRARIES = libkonq.la
libkonq_la_LDFLAGS = $(all_libraries) -version-info 3:0
libkonq_la_LIBADD = -lkded $(LIB_KFILE) -lkpartsui -lkparts

libkonq_la_SOURCES = kbookmarkmenu.cc kbookmark.cc knewmenu.cc kpopupmenu.cc \
	kpropsdlg.cc \
	kdirlister.cc kfileitem.cc kfileivi.cc qiconview.cpp qiconview_hack.cc
#	kdirlister.cc kfileitem.cc kiconcontainer.cc kicondrag.cc kfileivi.cc

METASOURCES = kbookmark.moc kbookmarkmenu.moc knewmenu.moc kpopupmenu.moc \
	kpropsdlg.moc \
	kdirlister.moc kfileivi.moc 
#	kdirlister.moc kiconcontainer.moc kicondrag.moc

include_HEADERS = kbookmark.h kbookmarkmenu.h knewmenu.h kpopupmenu.h \
	kpropsdlg.h \
	kdirlister.h kfileitem.h kfileivi.h qiconview.h
#	kdirlister.h kfileitem.h kiconcontainer.h kicondrag.h kfileivi.h

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot *.cc *.h && mv messages.po ../po/libkonq.pot

qiconview.moc: $(srcdir)/qiconview.h
	$(MOC) $(srcdir)/qiconview.h -o qiconview.moc

qiconview.moc.cpp: $(srcdir)/qiconview.cpp
	$(MOC) $(srcdir)/qiconview.cpp -o qiconview.moc.cpp

qiconview.lo : qiconview.moc.cpp qiconview.moc qiconview.cpp
