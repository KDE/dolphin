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

#include "kbookmarkimporter_crash.h"
#include <kfiledialog.h>
#include <kstringhandler.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <qfile.h>
#include <qdir.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <dcopclient.h>

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

#define LINELIMIT 4096

void KCrashBookmarkImporter::parseCrashLog( QString filename, bool del )
{
    QFile f(filename);

    if(f.open(IO_ReadOnly)) {

        QCString s(4096);

        QTextCodec * codec = QTextCodec::codecForName("UTF-8");
        Q_ASSERT(codec);
        if (!codec) return;

        typedef QMap<QString, QString> ViewMap;
        ViewMap views;

        while(f.readLine(s.data(), LINELIMIT)>=0) {
            if ( s[s.length()-1] != '\n' ) // Gosh, this line is longer than LINELIMIT. Skipping.
            {
               kdWarning() << "Crash bookmarks contain a line longer than " << LINELIMIT << ". Skipping." << endl;
               continue;
            }
            // KStringHandler::csqueeze()
            QString t = codec->toUnicode(s.stripWhiteSpace());
            QRegExp rx( "(.*)\\((.*)\\):(.*)$" );
            rx.setMinimal( TRUE );
            if (rx.exactMatch(t)) {
               if (rx.cap(1) == "opened") {
                  views[rx.cap(2)] = rx.cap(3);
               } else if (rx.cap(1) == "close") {
                  views.remove(rx.cap(2));
               }
            }
        }

        for ( ViewMap::Iterator it = views.begin(); it != views.end(); ++it ) {
           emit newBookmark( it.data(), it.data().latin1(), QString("") );
        }

        f.close();

        if (del) f.remove();
    }
}

QStringList KCrashBookmarkImporter::getCrashLogs() {

   QDir d( crashBookmarksDir() );
   d.setFilter( QDir::Files );
   d.setNameFilter("konqueror-crash-*.log");

   QMap<QString, bool> activeLogs;

   DCOPClient* dcop = kapp->dcopClient();

   QCStringList apps = dcop->registeredApplications();
   for ( QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it )
   {
      QCString &clientId = *it;

      if ( qstrncmp(clientId, "konqueror", 9) != 0 ) 
         continue;

      QByteArray data, replyData;
      QCString replyType;
      QDataStream arg(data, IO_WriteOnly);

      if ( !dcop->call( clientId.data(), "KonquerorIface", "crashLogFile()", data, replyType, replyData) ) {
         kdWarning() << "can't find dcop function KonquerorIface::crashLogFile()" << endl;
      } else {
         QDataStream reply(replyData, IO_ReadOnly);

         if ( replyType == "QString" )
         {
            QString ret;
            reply >> ret;
            activeLogs[ret] = true; // AK - nicer way to put this?
         }
      }
   }

   const QFileInfoList *list = d.entryInfoList();
   QFileInfoListIterator it( *list );

   QFileInfo *fi;
   QStringList crashFiles;

   for ( ; (fi = it.current()) != 0; ++it ) {
      bool dead = !activeLogs.contains(fi->absFilePath());
      if (dead) {
         crashFiles << fi->absFilePath();
      }
   }

   return crashFiles;
}

void KCrashBookmarkImporter::parseCrashBookmarks( bool del )
{
   QStringList crashFiles = KCrashBookmarkImporter::getCrashLogs();
   int len = crashFiles.count();
   int n = 1;

   for ( QStringList::Iterator it = crashFiles.begin(); it != crashFiles.end(); ++it ) {
      if (len > 1) {
         emit newFolder( QString("Instance %1").arg(n++), false, "" );
      }
      parseCrashLog(*it, del);
      if (len > 1) {
         emit endFolder();
      }
   }
}

QString KCrashBookmarkImporter::crashBookmarksDir( )
{
   return locateLocal("tmp", "");
}

#include "kbookmarkimporter_crash.moc"
