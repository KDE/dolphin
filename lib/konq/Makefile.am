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

INCLUDES = $(all_includes) -I$(top_srcdir)/kcontrol/input

lib_LTLIBRARIES = libkonq.la
libkonq_la_LDFLAGS = $(all_libraries) -version-info 3:0 -no-undefined
libkonq_la_LIBADD = $(LIB_KFILE)

libkonq_la_SOURCES = kbookmarkmenu.cc kbookmark.cc knewmenu.cc konqpopupmenu.cc \
	kpropsdlg.cc \
	kdirlister.cc konqfileitem.cc kfileivi.cc \
	konqsettings.cc konqdrag.cc konqiconviewwidget.cc \
	konqoperations.cc kbookmarkbar.cc


METASOURCES = kbookmark.moc kbookmarkmenu.moc knewmenu.moc konqpopupmenu.moc \
	kpropsdlg.moc \
	kdirlister.moc kfileivi.moc konqdrag.moc konqiconviewwidget.moc \
	konqoperations.moc kbookmarkbar.moc

include_HEADERS = kbookmark.h kbookmarkmenu.h knewmenu.h konqpopupmenu.h \
	kpropsdlg.h \
	kdirlister.h konqfileitem.h kfileivi.h \
	konqdefaults.h konqsettings.h konqdrag.h konqiconviewwidget.h \
	konqoperations.h kbookmarkbar.h

check_PROGRAMS = linktest
linktest_SOURCES = $(libkonq_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(libkonq_la_LIBADD) 
messages:
	$(XGETTEXT) *.cc *.h -o $(podir)/libkonq.pot

