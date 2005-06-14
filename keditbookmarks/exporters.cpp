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
}

void HTMLExporter::write(const KBookmarkGroup &grp, const QString &filename, bool showAddress) {
    QFile file(filename);
    if (!file.open(IO_WriteOnly)) {
        kdError(7043) << "Can't write to file " << filename << endl;
        return;
    }
    QTextStream tstream(&file);
    tstream.setEncoding(QTextStream::UnicodeUTF8);
    tstream << toString(grp, showAddress);
}

QString HTMLExporter::toString(const KBookmarkGroup &grp, bool showAddress)
{
    m_showAddress = showAddress;
    traverse(grp);
    return "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
           "<html><head><title>"+i18n("My Bookmarks")+"</title>\n"
           "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
           "</head>\n"
           "<body>\n"
           "<div>"
         + m_string +
           "</div>\n"
           "</body>\n</html>\n";
}

void HTMLExporter::visit(const KBookmark &bk) {
    // kdDebug() << "visit(" << bk.text() << ")" << endl;
    if(bk.isSeparator())
    {
        m_out << bk.fullText() << "<br>"<<endl;
    }
    else
    {
        if(m_showAddress)
        {
            m_out << bk.fullText() <<"<br>"<< endl;
            m_out << "<i><div style =\"margin-left: 1em\">" << bk.url().url().utf8() << "</div></i>";
        }
        else
        {
            m_out << "<a href=\"" << bk.url().url().utf8() << "\">";
            m_out << bk.fullText() << "</a><br>" << endl;
        }
    }
}

void HTMLExporter::visitEnter(const KBookmarkGroup &grp) {
    // kdDebug() << "visitEnter(" << grp.text() << ")" << endl;
    m_out << "<b>" << grp.fullText() << "</b><br>" << endl;
    m_out << "<div style=\"margin-left: 2em\">"<< endl;
} 

void HTMLExporter::visitLeave(const KBookmarkGroup &) {
    // kdDebug() << "visitLeave()" << endl;
    m_out << "</div>" << endl;
}

