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
#include <qfile.h>
#include <qdir.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <dcopclient.h>

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

/*
#!/home/kelletta/install/ruby/bin/ruby

logdir = "/tmp/kde-kelletta"
logprefix = "konqueror-crashlog";

sessions = []

Dir.foreach(logdir) {
   |fname|
   sessions << "#{logdir}/#{$1}" if fname =~ /^(#{logprefix}.*)$/
}

crashxbel = File.open("/tmp/crash.xml","w")

crashxbel.print "<!DOCTYPE xbel>\n"
crashxbel.print "<xbel>\n"

sessions.each {

   |session|

   id = /^.*?\/#{logprefix}([^\/]*)$/.match(session)[1];

   map = {}

   IO.foreach(session) {
      |line|
      if line =~ /^(.*?)\((.*?)\) == (.*)$/
         if $1 == "opened"
            map[$2] = $3
         elsif $1 == "close"
            map.delete($2)
         end
      end
   }

   crashxbel.print "  <folder>\n"
   crashxbel.print "    <title>#{id}</title>\n"

   map.each {
      |viewid, url|
      crashxbel.print "      <bookmark icon=\"html\" href=\"#{url}\">\n"
      crashxbel.print "        <title>#{url}</title>\n"
      crashxbel.print "      </bookmark>\n"
   }

   crashxbel.print "  </folder>\n"

   $stderr.print "would unlink(#{session})\n";

}

crashxbel.print "</xbel>\n"

crashxbel.close
*/

#define LINELIMIT 4096

void KCrashBookmarkImporter::parse_crash_file( QString filename )
{
    QFile f(filename);

    /*
    // TODO - what sort of url's can we get???
    QTextCodec * codec = QTextCodec::codecForName("UTF-8");
    Q_ASSERT(codec);
    if (!codec)
        return;
    */

    if(f.open(IO_ReadOnly)) {

        QCString s(4096);

        typedef QMap<QString, QString> ViewMap;
        ViewMap views;

        while(f.readLine(s.data(), LINELIMIT)>=0) {
            if ( s[s.length()-1] != '\n' ) // Gosh, this line is longer than LINELIMIT. Skipping.
            {
               kdWarning() << "Crash bookmarks contain a line longer than " << LINELIMIT << ". Skipping." << endl;
               continue;
            }
            QCString t = s.stripWhiteSpace();
            QRegExp rx( "(.*)\\((.*)\\) \\=\\= (.*)$" );
            rx.setMinimal( TRUE );
            rx.exactMatch(t);
            if (rx.cap(1) == "opened") {
               views[rx.cap(2)] = rx.cap(3);
            }
            /* else if (rx.cap(1) == "close") {
               views.remove(rx.cap(2));
               error message here???
            } */
        }

        // if (nViews > 1)
        emit newFolder( "View 1", false, "" );

        for ( ViewMap::Iterator it = views.begin(); it != views.end(); ++it ) {
           emit newBookmark( it.data(), it.data().latin1(), QString("") );
        }

        emit endFolder();

        f.close();
    }
}

void KCrashBookmarkImporter::parseCrashBookmarks( )
{
   typedef QMap<QString, bool> FileMap;
   FileMap activeLogs;

   // for n in `dcop  | grep konqueror`; do dcop $n KonquerorIface 'crashLogFile()'; done
   /*
   for (each konqi instance) {
   }
   */

   DCOPClient* dcop = kapp->dcopClient();

   QCStringList apps = dcop->registeredApplications();
   for ( QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it )
   {
      QCString &clientId = *it;

      if ( (clientId == dcop->appId()) 
        || qstrncmp(clientId, "konqueror", 9) != 0
      ) {
         continue;
      }

      QByteArray data, replyData;
      QCString replyType;
      QDataStream arg(data, IO_WriteOnly);

      if ( !dcop->call( clientId.data(), "KonquerorIface", "crashLogFile()", data, replyType, replyData) ) {
         kdWarning() << "umm.. you've probably not got my patched konqi :)" << endl;
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

   QDir d(m_fileName);
   d.setFilter( QDir::Files );
   d.setNameFilter("konqueror-crashlog*.xml");

   const QFileInfoList *list = d.entryInfoList();
   QFileInfoListIterator it( *list );
   QFileInfo *fi;

   while ( (fi = it.current()) != 0 ) {
      ++it;

      bool stillActive = activeLogs.contains(fi->absFilePath());

      if (!stillActive) {
         parse_crash_file(fi->absFilePath());
      }
   }
}

QString KCrashBookmarkImporter::crashBookmarksDir( )
{
    return KFileDialog::getExistingDirectory( );
}

#include "kbookmarkimporter_crash.moc"
