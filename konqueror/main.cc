#include "main.h"
#include "kio_job.h"
#include "kio_cache.h"
#include "kfmgui.h"
#include "kfmpaths.h"
#include "kmimetypes.h"
#include "kmimemagic.h"
#include "kservices.h"
#include "kuserprofile.h"
#include "xview.h"
#include "kregistry.h"
#include "kregfactories.h"

#include <kio_manager.h>
#include <khtml.h>
#include <kimgio.h>
#include <kapp.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void sig_handler( int signum );
void sig_term_handler( int signum );
void sig_pipe_handler( int signum );

/*
int main( int argc, char **argv )
{
  ProtocolManager manager;
  
  KApplication app( argc, argv );
  
  assert( argc >= 2 );
  
  if ( strcmp( argv[1], "cp" ) == 0 )
  {
    assert( argc == 4 );

    KIOJob* job = new KIOJob;
    job->copy( argv[ 2 ], argv[ 3 ] );
  }
  else if ( strcmp( argv[1], "get" ) == 0 )
  {
    assert( argc == 3 );

    KIOJob* job = new KIOJob;
    job->get( argv[ 2 ] );
  }

  app.exec();
}
*/

void KfmBookmarkManager::editBookmarks( const char *_url )
{
  KfmGui* gui = new KfmGui( _url );
  gui->show();
}

#include "kiconcontainer.h"
#include "kpixmapcache.h"

int main( int argc, char **argv )
{
  KApplication app( argc, argv );

  signal(SIGCHLD,sig_handler);
  signal(SIGTERM,sig_term_handler);
  signal(SIGPIPE,sig_pipe_handler);

  ProtocolManager manager;

  KfmPaths::initStatic();
  KIOJob::initStatic();
  KIOCache::initStatic();
  KMimeType::initStatic();
  KMimeMagic::initStatic();
  KService::initStatic();
  KServiceTypeProfile::initStatic();

  KRegistry registry;
  registry.addFactory( new KMimeTypeFactory );
  KMimeType::check();
  registry.addFactory( new KServiceFactory );
  
  kimgioRegister();
  
  QImageIO::defineIOHandler( "XV", "^P7 332", 0, read_xv_file, 0L );

  KfmBookmarkManager bm;

  KfmGui* gui = new KfmGui( "file:/tmp" );
  gui->show();
  app.exec();
}

void sig_handler( int )
{
    int pid;
    int status;
    
    while( 1 )
    {
	pid = waitpid( -1, &status, WNOHANG );
	if ( pid <= 0 )
	{
	    // Reinstall signal handler, since Linux resets to default after
	    // the signal occured ( BSD handles it different, but it should do
	    // no harm ).
	    signal( SIGCHLD, sig_handler );
	    return;
	}
    }
}

void sig_term_handler( int )
{
  printf("###################### TERM: Deleting sockets ###################\n");

  /*
  if ( pkfm->isGoingDown() )
    return;
  
  // Save cache and stuff and delete the sockets ...
  pkfm->slotSave();
  pkfm->slotShutDown();
  */

  exit(1);
}

void sig_pipe_handler( int )
{
  printf("###################### SIG Pipe ###################\n");
  signal( SIGPIPE, sig_pipe_handler );
}
