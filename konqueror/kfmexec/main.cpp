#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <kapp.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kio_job.h>
#include <kio_netaccess.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include <qlist.h>
#include <qstring.h>

#include "main.h"


static const char *description = 
	I18N_NOOP("KFM Exec - A Location opener");

static const char *version = "v0.0.1";

static KCmdLineOptions options[] =
{
   { "+command", I18N_NOOP("Command to execute."), 0 },
   { "[URLs]", I18N_NOOP("URL(s) or local file used for 'command'."), 0 },
   { 0, 0, 0 }
};


int jobCounter = 0;

template class QList<KIOJob>;

QList<KIOJob>* jobList = 0L;

KFMExec::KFMExec()
{
    jobList = new QList<KIOJob>;
    jobList->setAutoDelete( false ); // jobs autodelete themselves

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->count() < 1)
       KCmdLineArgs::usage(i18n("'command' expected.\n"));

    expectedCounter = 0;
    command = args->arg(0);
    kDebugString( command );

    for ( int i = 1; i < args->count(); i++ )
    {
        QCString arg = args->arg(i);
        // A local file, not an URL ?
	// => It is not encoded and not shell escaped, too.
	if ( arg[0] == '/' )
	{
	    QCString tmp( shellQuote( arg ) );
	    if ( !files.isEmpty() )
		files += " ";
	    files += "\"";
	    files += tmp.data();
	    files += "\"";
	}
	// It is an URL
	else
        {
	    KURL u( arg );
	    if ( u.isMalformed() )
	    {
                KMessageBox::error( 0L, i18n( "The URL %1\nis malformed" ).arg( arg ) );
	    }
	    // Must we fetch the file ?
	    else if ( !u.isLocalFile() )
	    {
		KIOJob *job = new KIOJob;
		jobList->append( job );

		connect( job, SIGNAL( sigFinished( int ) ), SLOT( slotFinished() ) );
                // Go to finished on error (we could do better but at least we don't want to wait forever)
		connect( job, SIGNAL( sigError( int, int, const char * ) ), SLOT( slotFinished() ) );
	
                // Build the destination filename, in ~/.kde/share/apps/kfmexec/tmp/
                // Unlike KDE-1.1, we put the filename at the end so that the extension is kept
                // (Some programs rely on it)
		QString tmp = locateLocal( "appdata", "tmp/" ) +
		              QString("%1.%2.%3").arg(getpid()).arg(jobCounter++).arg(u.filename());
		if ( !files.isEmpty() )
		    files += " ";
		files += "\"";
		files += tmp;
		files += "\"";
		fileList.append( tmp );
		urlList.append( arg );

		expectedCounter++;
		job->copy( arg, tmp );
	    }
	    else // It is a local file system URL
	    {
		QString tmp1( u.path() );
		// ? KURL::decodeURL( tmp1 );
		QString tmp( shellQuote( tmp1 ) );
		if ( !files.isEmpty() )
		    files += " ";
		files += "\"";
		files += tmp.data();
		files += "\"";
	    }
	}
    }
    args->clear();

    counter = 0;
    if ( counter == expectedCounter )
	slotFinished();
}

void KFMExec::slotFinished()
{
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

    kDebugInfo(0, "EXEC '%s'\n",command.data() );

    system( command );

    // Test whether one of the files changed
    i = 0;
    QStringList::ConstIterator urlIt = urlList.begin();
    it = fileList.begin();
    for ( ; it != fileList.end() ; ++it, ++urlIt )
    {
	struct stat buff;
        QString src = *it;
        QString dest = *urlIt;
	if ( stat( src, &buff ) == 0 && times[i++] != buff.st_mtime )
	{
	    if ( KMessageBox::questionYesNo( 0L, i18n( "The file\n%1\nhas been modified.\nDo you want to save it?" ).arg(src) )
                      == KMessageBox::Yes )
	    {
		kDebugString(QString("src='%1'  dest='%2'").arg(src).arg(dest));
                // Do it the synchronous way.
		KIONetAccess::upload( src, dest );
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

QString KFMExec::shellQuote( const char *_data )
{
    QString cmd = _data;

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

    return QString( cmd.data() );
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
