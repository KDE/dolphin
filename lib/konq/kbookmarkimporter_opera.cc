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

#include "kbookmarkimporter_opera.h"
#include <kfiledialog.h>
#include <kstringhandler.h>
#include <klocale.h>
#include <kdebug.h>
#include <qtextcodec.h>

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

#define LINELIMIT 4096

// TODO - what sort of url's can we get???
// QTextCodec * codec = QTextCodec::codecForName("UTF-8");
// Q_ASSERT(codec);
// if (!codec)
//   return;

void KOperaBookmarkImporter::parseOperaBookmarks( )
{
   QString URL = QString::null;
   QString NAME = QString::null;
   QString TYPE = QString::null;

   QFile f(m_fileName);

   if(f.open(IO_ReadOnly)) {

      QCString s(4096);

      int lineno = 0;

      while(f.readLine(s.data(), LINELIMIT)>=0) {

        lineno++;

        if ( s[s.length()-1] != '\n' ) // Gosh, this line is longer than LINELIMIT. Skipping.
        {
            kdWarning() << "IE bookmarks contain a line longer than " << LINELIMIT << ". Skipping." << endl;
            continue;
        }
        if (lineno <= 2) continue; // skip first two header lines

        QString currentLine = s.stripWhiteSpace();
        // kdWarning() << currentLine << endl;

        if ( currentLine == "" ) {

           if ( TYPE != QString::null 
             && TYPE == "URL" 
           ) {
              emit newBookmark( NAME, URL.latin1(), QString("") );
              TYPE = QString::null;
              NAME = QString::null;
              URL = QString::null;
           } else if ( 
                TYPE != QString::null
             && TYPE == "FOLDER" ) 
           {
              emit newFolder( NAME, false, "" ); 
              TYPE = QString::null;
              NAME = QString::null;
              URL = QString::null;
           }
           
        } else if (currentLine == "-") {
           emit endFolder();

        } else {
           QString tag;

           if ( tag = "#", currentLine.startsWith( tag ) ) {
              TYPE = currentLine.remove( 0, tag.length() );
              // kdWarning() << "TYPE == " << TYPE << endl;

           } else 
           if ( tag = "NAME=", currentLine.startsWith( tag ) ) {
              NAME = currentLine.remove( 0, tag.length() );
              // convertEntities?
              // kdWarning() << "NAME == " << NAME << endl;

           } else 
           if ( tag = "URL=", currentLine.startsWith( tag ) ) {
              URL = currentLine.remove( 0, tag.length() );
              // kdWarning() << "URL == " << URL << endl;
           }
        }
     }
   }

}

QString KOperaBookmarkImporter::operaBookmarksFile( )
{
    return KFileDialog::getOpenFileName( QDir::homeDirPath() + "/.opera", 
                                         i18n("*.adr|Opera bookmark files (*.adr)") );
}

#include "kbookmarkimporter_opera.moc"
