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

// Note - KNSBookmarkImporter can be used to get the path to the filename

class KEBBookmarkExporterBase
{
public:
    KEBBookmarkExporterBase(KBookmarkManager* mgr, const QString & fileName)
        : m_fileName(fileName), m_pManager(mgr) 
    { ; }
    virtual ~KEBBookmarkExporterBase() {}
    virtual void write(bool utf8 /* true for mozilla */, KBookmarkGroup) = 0;
protected:
    virtual const QString folderAsString(KBookmarkGroup) = 0;
    QString m_fileName;
    KBookmarkManager* m_pManager;
};

class KEBNSBookmarkExporterImpl : public KEBBookmarkExporterBase
{
public:
    KEBNSBookmarkExporterImpl(KBookmarkManager* mgr, const QString & fileName)
      : KEBBookmarkExporterBase(mgr, fileName) 
    { ; }
    virtual ~KEBNSBookmarkExporterImpl() {}
    virtual void write(bool utf8, KBookmarkGroup);
protected:
    virtual const QString folderAsString(KBookmarkGroup);
};

// BC - remove for KDE 4.0
class KEBNSBookmarkExporter
{
public:
    KEBNSBookmarkExporter(KBookmarkManager* mgr, const QString & fileName)
      : m_fileName(fileName), m_pManager(mgr) 
    { ; }
    ~KEBNSBookmarkExporter() {}

    void write( bool utf8 );
    void write() { write(false); } // deprecated

protected:
    void writeFolder(QTextStream &stream, KBookmarkGroup parent);
    QString m_fileName;
    KBookmarkManager* m_pManager;
};
#endif
