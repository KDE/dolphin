#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

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
    kDebugInfo( command.ascii() );

    for ( int i = 1; i < args->count(); i++ )
    {
        KURL url = args->url(i);
        // A local file, not an URL ?
	// => It is not encoded and not shell escaped, too.
	if ( url.isLocalFile() )
	{
	    QString tmp( shellQuote( url.path() ) );
	    if ( !files.isEmpty() )
		files += " ";
	    files += "\"";
	    files += tmp.local8Bit();
	    files += "\"";
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
		              QString("%1.%2.%3").arg(getpid()).arg(jobCounter++).arg(url.filename());
		if ( !files.isEmpty() )
		    files += " ";
		files += "\"";
		files += tmp;
		files += "\"";
		fileList.append( tmp );
		urlList.append( url );

		expectedCounter++;
		KIO::Job *job = KIO::copy( url, tmp );
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
        job->showErrorDialog();

    counter++;

    if ( counter < expectedCounter )
	return;

    jobList->clear();

    int i = 0;
    while ( ( i = command.find( "%f", i ) ) != -1 )
    {
        command.replace( i, 2, files );
        i += files.length();
    }

    // Store modification times
    int* times = new int[ fileList.count() ];
    i = 0;
    QStringList::ConstIterator it = fileList.begin();
    for ( ; it != fileList.end() ; ++it )
    {
	struct stat buff;
	stat( *it, &buff );
	times[i++] = buff.st_mtime;
    }

    kDebugInfo("EXEC '%s'\n", debugString(command) );

    system( command );

    // Test whether one of the files changed
    i = 0;
    KURL::List::ConstIterator urlIt = urlList.begin();
    it = fileList.begin();
    for ( ; it != fileList.end() ; ++it, ++urlIt )
    {
	struct stat buff;
        QString src = *it;
        KURL dest = *urlIt;
	if ( stat( src, &buff ) == 0 && times[i++] != buff.st_mtime )
	{
	    if ( KMessageBox::questionYesNo( 0L, i18n( "The file\n%1\nhas been modified.\nDo you want to save it?" ).arg(src) )
                      == KMessageBox::Yes )
	    {
		kDebugInfo(QString("src='%1'  dest='%2'").arg(src).arg(dest.url()).ascii());
                // Do it the synchronous way.
		KIO::NetAccess::upload( src, dest );
	    }
	}
	else
	{
	    unlink( src.local8Bit() );
	}
    }

    jobList->clear();
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
        kDebugFatal( i18n( "Syntax Error:\nkfmexec command [URLs ....]" ) );
	exit(1);
    }

    KFMExec exec;

    app.exec();

    return 0;
}

#include "main.moc"
