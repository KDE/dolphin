/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef __kbookmarkimporter_h
#define __kbookmarkimporter_h

#include <qdom.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <ksimpleconfig.h>

/**
 * A class for importing the previous bookmarks (desktop files)
 * Separated from KBookmarkManager to save memory (we throw this one
 * out once the import is done)
 */
class KBookmarkImporter
{
public:
    KBookmarkImporter( QDomDocument * doc ) : m_pDoc(doc) {}

    void import( const QString & path );

private:
    void scanIntern( QDomElement & parentElem, const QString & _path );
    void parseBookmark( QDomElement & parentElem, QCString _text,
                        KSimpleConfig& _cfg, const QString &_group );
    QDomDocument * m_pDoc;
    QStringList m_lstParsedDirs;
};

/**
 * A class for importing NS bookmarks
 * KEditBookmarks uses it to insert bookmarks into its DOM tree,
 * and KActionMenu uses it to create actions directly.
 */
class KNSBookmarkImporter : public QObject
{
    Q_OBJECT
public:
    KNSBookmarkImporter( const QString & fileName ) : m_fileName(fileName) {}
    ~KNSBookmarkImporter() {}

    // for compat reasons only
    void parseNSBookmarks() { parseNSBookmarks(false); }
    // go for it. Set utf8 to true for Mozilla, false for Netscape.
    void parseNSBookmarks( bool utf8 );

    // Usual place for NS bookmarks
    static QString netscapeBookmarksFile( bool forSaving=false );
    // Usual place for Mozilla bookmarks
    static QString mozillaBookmarksFile( bool forSaving=false );

signals:

    /**
     * Notify about a new bookmark
     * Use "html" for the icon
     */
    void newBookmark( const QString & text, const QCString & url, const QString & additionnalInfo );

    /**
     * Notify about a new folder
     * Use "folder" for the icon
     */
    void newFolder( const QString & text, bool open, const QString & additionnalInfo );

    /**
     * Notify about a new separator
     */
    void newSeparator();

    /**
     * Tell the outside world that we're going down
     * one menu
     */
    void endMenu();

protected:
    QString m_fileName;
};

#endif
