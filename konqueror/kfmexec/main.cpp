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
#include <qlist.h>
#include <qstring.h>

#include "main.h"

int jobCounter = 0;

QList<KIOJob>* jobList = 0L;

KFMExec::KFMExec( int argc, char **argv )
{
    jobList = new QList<KIOJob>;
    jobList->setAutoDelete( false ); // jobs autodelete themselves

    expectedCounter = 0;
    command = argv[ 1 ];
    kDebugString( command );

    for ( int i = 3; i <= argc; i++ )
    {
        // A local file, not an URL ?
	// => It is not encoded and not shell escaped, too.
	if ( *argv[ i - 1 ] == '/' )
	{
	    QCString tmp( shellQuote( argv[ i - 1 ] ) );
	    if ( !files.isEmpty() )
		files += " ";
	    files += "\"";
	    files += tmp.data();
	    files += "\"";
	}
	// It is an URL
	else
        {
	    KURL u( argv[ i - 1 ] );
	    if ( u.isMalformed() )
	    {
                KMessageBox::error( 0L, i18n( "The URL %1\nis malformed" ).arg( argv[ i - 1 ] ) );
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
		urlList.append( argv[ i - 1 ] );

		expectedCounter++;
		job->copy( argv[ i - 1 ], tmp );
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
    KApplication app( argc, argv, "kfmexec" );

    if ( argc < 2 )
    {
        kDebugFatal( i18n( "Syntax Error:\nkfmexec command [URLs ....]" ) );
	exit(1);
    }

    KFMExec exec(argc, argv);

    app.exec();

    return 0;
}

#include "main.moc"
