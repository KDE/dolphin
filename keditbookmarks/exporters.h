/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __exporters_h
#define __exporters_h

#include <kbookmark.h>
//Added by qt3to4:
#include <QTextStream>

class HTMLExporter : private KBookmarkGroupTraverser {
public:
   HTMLExporter();
   virtual ~HTMLExporter(){}
   QString toString(const KBookmarkGroup &, bool showAddress = false);
   void write(const KBookmarkGroup &, const QString &, bool showAddress = false);
private:
   virtual void visit(const KBookmark &);
   virtual void visitEnter(const KBookmarkGroup &);
   virtual void visitLeave(const KBookmarkGroup &);
private:
   QString m_string;
   QTextStream m_out;
   bool m_showAddress;
};

#endif
