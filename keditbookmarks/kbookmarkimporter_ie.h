/* This file is part of the KDE libraries
   Copyright (C) 2002 Alexander Kellett <lypanov@kde.org>

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

#ifndef __kbookmarkimporter_ie_h
#define __kbookmarkimporter_ie_h

#include <qdom.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <ksimpleconfig.h>

/**
 * A class for importing NS bookmarks
 * KEditBookmarks uses it to insert bookmarks into its DOM tree,
 * and KActionMenu uses it to create actions directly.
 */
class KIEBookmarkImporter : public QObject
{
    Q_OBJECT
public:
    KIEBookmarkImporter( const QString & fileName ) : m_fileName(fileName) {}
    ~KIEBookmarkImporter() {}

    void parseIEBookmarks();

    // Usual place for IE bookmarks
    static QString IEBookmarksDir();

signals:

    /**
     * Notify about a new bookmark
     * Use "html" for the icon
     */
    void newBookmark( const QString & text, const QCString & url, const QString & additionalInfo );

    /**
     * Notify about a new folder
     * Use "bookmark_folder" for the icon
     */
    void newFolder( const QString & text, bool open, const QString & additionalInfo );

    /**
     * Notify about a new separator
     */
    void newSeparator();

    /**
     * Tell the outside world that we're going down
     * one menu
     */
    void endFolder();

protected:
    void parseIEBookmarks_dir( QString dirname, QString name = "" );
    void parseIEBookmarks_url_file( QString filename, QString name );

    QString m_fileName;

};

#endif
