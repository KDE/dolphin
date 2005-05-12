// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "exporters.h"

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>

#include <qfile.h>

HTMLExporter::HTMLExporter() 
    : m_out(&m_string, IO_WriteOnly) {
    m_level = 0;
}

void HTMLExporter::write(const KBookmarkGroup &grp, const QString &filename) {
    QFile file(filename);
    if (!file.open(IO_WriteOnly)) {
        kdError(7043) << "Can't write to file " << filename << endl;
        return;
    }
    QTextStream tstream(&file);
    tstream.setEncoding(QTextStream::UnicodeUTF8);
    tstream << toString(grp);
}

QString HTMLExporter::toString(const KBookmarkGroup &grp)
{
    traverse(grp);
    return "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
           "<HTML><HEAD><TITLE>My Bookmarks</TITLE>\n"
           "<META http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\" />"
           "</HEAD>\n"
           "<BODY>\n"
         + m_string +
           "</BODY></HTML>\n";
}

void HTMLExporter::visit(const KBookmark &bk) {
    // kdDebug() << "visit(" << bk.text() << ")" << endl;
    m_out << "<A href=\"" << bk.url().url().utf8() << "\">";
    m_out << bk.fullText() << "</A><BR>" << endl;
}

void HTMLExporter::visitEnter(const KBookmarkGroup &grp) {
    // kdDebug() << "visitEnter(" << grp.text() << ")" << endl;
    m_out << "<H3>" << grp.fullText() << "</H3>" << endl;
    m_out << "<P style=\"margin-left: " 
          << (m_level * 3) << "em\">" << endl;
    m_level++;
} 

void HTMLExporter::visitLeave(const KBookmarkGroup &) {
    // kdDebug() << "visitLeave()" << endl;
    m_out << "</P>" << endl;
    m_level--;
    if (m_level != 0)
        m_out << "<P style=\"left-margin: " 
              << (m_level * 3) << "em\">" << endl;
}

