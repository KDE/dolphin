/* This file is part of the KDE libraries
   Copyright (C) 1996-1998 Martin R. Jones <mjones@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kebbookmarkexporter_h
#define __kebbookmarkexporter_h

#include <qtextstream.h>
#include <kbookmark.h>

class KEBBookmarkExporterBase
{
public:
    // use KNSBookmarkImporter to get the path to the filename
    KEBBookmarkExporterBase( KBookmarkManager* mgr,const QString & fileName )
        : m_fileName(fileName), m_pManager(mgr) {}
    virtual ~KEBBookmarkExporterBase() {}

    // Write out. Use utf8=true for Mozilla, false for Netscape
    virtual void write( bool utf8 ) = 0;

protected:
    virtual void writeFolder( QTextStream &stream, KBookmarkGroup parent ) = 0;
    QString m_fileName;
    KBookmarkManager* m_pManager;
};

/**
 * A class that exports all the current bookmarks to Netscape/Mozilla bookmarks
 * Warning, it overwrites the existing bookmarks.html file !
 */
class KEBNSBookmarkExporterImpl : public KEBBookmarkExporterBase
{
public:
    // use KNSBookmarkImporter to get the path to the filename
    KEBNSBookmarkExporterImpl( KBookmarkManager* mgr,const QString & fileName )
      : KEBBookmarkExporterBase( mgr, fileName ) {}
    virtual ~KEBNSBookmarkExporterImpl() {}

    virtual void write( bool utf8 );

protected:
    virtual void writeFolder( QTextStream &stream, KBookmarkGroup parent );
};

/**
 * A class that exports all the current bookmarks to Netscape/Mozilla bookmarks
 * Warning, it overwrites the existing bookmarks.html file !
 */
class KEBNSBookmarkExporter
{
public:
    // use KNSBookmarkImporter to get the path to the filename
    KEBNSBookmarkExporter( KBookmarkManager* mgr,const QString & fileName )
      : m_fileName(fileName), m_pManager(mgr) {}
    ~KEBNSBookmarkExporter() {}

    // For compat
    void write() { write(false); }
    // Write out. Use utf8=true for Mozilla, false for Netscape
    void write( bool utf8 );

protected:
    void writeFolder( QTextStream &stream, KBookmarkGroup parent );
    QString m_fileName;
    KBookmarkManager* m_pManager;
};
#endif
