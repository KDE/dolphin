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

#include "kbookmarkimporter_ie.h"
#include <kfiledialog.h>
#include <kstringhandler.h>
#include <klocale.h>
#include <kdebug.h>
#include <qtextcodec.h>

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

void KIEBookmarkImporter::parseIEBookmarks_url_file( QString filename, QString name ) {

    QFile f(filename);

    /*
    // TODO - what sort of url's can we get???
    QTextCodec * codec = QTextCodec::codecForName("UTF-8");
    Q_ASSERT(codec);
    if (!codec)
        return;
    */

    if(f.open(IO_ReadOnly)) {

#define LINELIMIT 4096
        QCString s(4096);

        typedef QMap<QString, QString> ViewMap;
        ViewMap views;

        while(f.readLine(s.data(), LINELIMIT)>=0) {
            if ( s[s.length()-1] != '\n' ) // Gosh, this line is longer than LINELIMIT. Skipping.
            {
               kdWarning() << "IE bookmarks contain a line longer than " << LINELIMIT << ". Skipping." << endl;
               continue;
            }
            QCString t = s.stripWhiteSpace();
            QRegExp rx( "URL=(.*)" );
            if (rx.exactMatch(t)) {
               emit newBookmark( name, rx.cap(1).latin1(), QString("") );
            }
        }

        f.close();
    }
}

void KIEBookmarkImporter::parseIEBookmarks_dir( QString dirname, QString name )
{

   if (dirname != m_fileName) 
      emit newFolder( name, false, "" );

   QDir d(dirname);
   d.setFilter( QDir::Files | QDir::Dirs );
   d.setSorting( QDir::Name | QDir::DirsFirst );
   d.setNameFilter("*.url;index.ini");
   d.setMatchAllDirs(TRUE);

   const QFileInfoList *list = d.entryInfoList();
   QFileInfoListIterator it( *list );
   QFileInfo *fi;

   while ( (fi = it.current()) != 0 ) {
      ++it;

      if (fi->fileName() == "." || fi->fileName() == "..") continue;

      if (fi->isDir()) {
         parseIEBookmarks_dir(fi->absFilePath(), fi->fileName());

      } else if (fi->isFile()) {
         if (fi->fileName().endsWith(".url")) {
            QString name = fi->fileName();
            name.truncate(name.length() - 4); // .url
            parseIEBookmarks_url_file(fi->absFilePath(), name);
         }
      }
   }

   if (dirname != m_fileName) 
      emit endFolder();
}


void KIEBookmarkImporter::parseIEBookmarks( )
{
    parseIEBookmarks_dir( m_fileName );
}

QString KIEBookmarkImporter::IEBookmarksDir( )
{
    return KFileDialog::getExistingDirectory( );
}

#include "kbookmarkimporter_ie.moc"
