/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qfile.h>

#include <kapp.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kstartupinfo.h>

#include <qlist.h>
#include <qstring.h>

#include "main.h"


static const char *description =
        I18N_NOOP("KFM Exec - Opens remote files, watches modifications, asks for upload");

static const char *version = "v0.0.1";

static KCmdLineOptions options[] =
{
   { "+command", I18N_NOOP("Command to execute."), 0 },
   { "[URLs]", I18N_NOOP("URL(s) or local file used for 'command'."), 0 },
   { 0, 0, 0 }
};


int jobCounter = 0;

QList<KIO::Job>* jobList = 0L;

KFMExec::KFMExec()
{
    jobList = new QList<KIO::Job>;
    jobList->setAutoDelete( false ); // jobs autodelete themselves

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->count() < 1)
       KCmdLineArgs::usage(i18n("'command' expected.\n"));

    expectedCounter = 0;
    command = args->arg(0);
    kdDebug() << command << endl;

    for ( int i = 1; i < args->count(); i++ )
    {
        KURL url = args->url(i);
        // A local file, not an URL ?
        // => It is not encoded and not shell escaped, too.
        if ( url.isLocalFile() )
        {
            QString tmp( shellQuote( url.path() ) );
            params.append(tmp);
        }
        // It is an URL
        else
        {
            if ( url.isMalformed() )
            {
                KMessageBox::error( 0L, i18n( "The URL %1\nis malformed" ).arg( url.url() ) );
            }
            // We must fetch the file
            else
            {
                // Build the destination filename, in ~/.kde/share/apps/kfmexec/tmp/
                // Unlike KDE-1.1, we put the filename at the end so that the extension is kept
                // (Some programs rely on it)
                QString tmp = locateLocal( "appdata", "tmp/" ) +
                              QString("%1.%2.%3").arg(getpid()).arg(jobCounter++).arg(url.fileName());
                params.append(tmp);
                fileList.append( tmp );
                urlList.append( url );

                expectedCounter++;
                KIO::Job *job = KIO::file_copy( url, tmp );
                jobList->append( job );

                connect( job, SIGNAL( result( KIO::Job * ) ), SLOT( slotResult( KIO::Job * ) ) );

            }
        }
    }
    args->clear();

    counter = 0;
    if ( counter == expectedCounter )
        slotResult( 0L );
}

void KFMExec::slotResult( KIO::Job * job )
{
    if (job && job->error())
    {
        job->showErrorDialog();
        QStringList::Iterator it = params.find( static_cast<KIO::FileCopyJob*>(job)->destURL().path() );
        if ( it != params.end() )
           params.remove( it );
        else
           kdDebug() <<  static_cast<KIO::FileCopyJob*>(job)->destURL().path() << " not found in list" << endl;
    }

    counter++;

    if ( counter < expectedCounter )
        return;

    // We know we can run the app now - but let's finish the job properly first.
    QTimer::singleShot( 0, this, SLOT( slotRunApp() ) );

    jobList->clear();
}

void KFMExec::slotRunApp()
{
    if ( params.isEmpty() )
      exit(1);

    int pos;
    while ( ( pos = command.find( "%f" )) != -1 )
      command.replace( pos, 2, "" );
    while ( ( pos = command.find( "%F" )) != -1 )
      command.replace( pos, 2, "" );

    command.append( " " );
    QString files;
    QStringList::ConstIterator cmdit = params.begin();
    for ( ; cmdit != params.end() ; ++cmdit ) 
    {
        if ( !files.isEmpty() )
            files += " ";
        files += "\"";
        files += *cmdit;
        files += "\"";
    }
    kdDebug() << "files= " << files << endl;
    command.append( files );

    // Store modification times
    int* times = new int[ fileList.count() ];
    int i = 0;
    QStringList::ConstIterator it = fileList.begin();
    for ( ; it != fileList.end() ; ++it )
    {
        struct stat buff;
        stat( QFile::encodeName(*it), &buff );
        times[i++] = buff.st_mtime;
    }

    kdDebug() << "EXEC '" << command << "'" << endl;

    // propagate the startup indentification to the started process
    KStartupInfoId id;
    id.initId( kapp->startupId());
    id.setupStartupEnv();

    system( QFile::encodeName(command) );

    KStartupInfo::resetStartupEnv();

    kdDebug() << "EXEC done" << endl;

    // Test whether one of the files changed
    i = 0;
    KURL::List::ConstIterator urlIt = urlList.begin();
    it = fileList.begin();
    for ( ; it != fileList.end() ; ++it, ++urlIt )
    {
        struct stat buff;
        QString src = *it;
        KURL dest = *urlIt;
        if ( stat( QFile::encodeName(src), &buff ) == 0 && times[i++] != buff.st_mtime )
        {
            if ( KMessageBox::questionYesNo( 0L,
                                             i18n( "The file\n%1\nhas been modified.\nDo you want to upload the changes?" ).arg(dest.prettyURL()),
                                             i18n("File changed" ) ) == KMessageBox::Yes )
            {
                kdDebug() << QString("src='%1'  dest='%2'").arg(src).arg(dest.url()).ascii() << endl;
                // Do it the synchronous way.
                if ( !KIO::NetAccess::upload( src, dest ) )
                {
                        KMessageBox::error( 0L, KIO::NetAccess::lastErrorString() );
                }
            }
        }
        else
        {
            unlink( src.local8Bit() );
        }
    }

    //kapp->quit(); not efficient enough
    exit(0);
}

QString KFMExec::shellQuote( const QString & data )
{
    QString cmd = data;
    int pos = 0;
    while ( ( pos = cmd.find( ";", pos )) != -1 )
    {
        cmd.replace( pos, 1, "\\;" );
        pos += 2;
    }
    pos = 0;
    while ( ( pos = cmd.find( "\"", pos )) != -1 )
    {
        cmd.replace( pos, 1, "\\\"" );
        pos += 2;
    }
    pos = 0;
    while ( ( pos = cmd.find( "|", pos ) ) != -1 )
    {
        cmd.replace( pos, 1, "\\|" );
        pos += 2;
    }
    pos = 0;
    while ( ( pos = cmd.find( "(", pos )) != -1 )
    {
        cmd.replace( pos, 1, "\\(" );
        pos += 2;
    }
    pos = 0;
    while ( ( pos = cmd.find( ")", pos )) != -1 )
    {
        cmd.replace( pos, 1, "\\)" );
        pos += 2;
    }

    return cmd;
}

int main( int argc, char **argv )
{
    KAboutData aboutData( "kfmexec", I18N_NOOP("KFMExec"),
        version, description, KAboutData::License_GPL,
        "(c) 1998-2000, The KFM/Konqueror Developers");
    aboutData.addAuthor("David Faure",0, "faure@kde.org");
    aboutData.addAuthor("Stephen Kulow",0, "coolo@kde.org");
    aboutData.addAuthor("Bernhard Rosenkraenzer",0, "bero@redhat.de");
    aboutData.addAuthor("Waldo Bastian",0, "bastian@kde.org");

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    if ( argc < 2 )
    {
        kdError() << i18n( "Syntax Error:\nkfmexec command [URLs ....]" ) << endl;
        exit(1);
    }

    KFMExec exec;

    kdDebug() << "Constructor returned..." << endl;
    return app.exec();
}

#include "main.moc"
