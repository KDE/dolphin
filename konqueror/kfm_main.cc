/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#include <qprinter.h>
#include <opApplication.h>
#include <komApplication.h>

#include <kmimetypes.h>
#include <kmimemagic.h>
#include <kservices.h>
#include <kuserprofile.h>
#include <kregistry.h>
#include <kregfactories.h>
#include <kio_job.h>
#include <kio_cache.h>
#include <kio_manager.h>
#include <khtml.h>
#include <kimgio.h>
#include <kapp.h>

#include <qdir.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include "kfm_main.h"
#include "kfm_mainwindow.h"
#include "kfmpaths.h"
#include "xview.h"
#include "kfmgui.h"
//#include "kfm_part.h"

// DEBUG
#include <iostream>

void sig_handler( int signum );
void sig_term_handler( int signum );
void sig_pipe_handler( int signum );

bool g_bWithGUI = true;

/**********************************************
 *
 * KfmApplicationIf
 *
 **********************************************/

typedef KOMBoot<KfmApplicationIf> KfmBoot;

KfmApplicationIf::KfmApplicationIf()
{
}

KfmApplicationIf::KfmApplicationIf( const CORBA::BOA::ReferenceData &refdata ) :
  KFM::Application_skel( refdata )
{
}

KfmApplicationIf::KfmApplicationIf( CORBA::Object_ptr _obj ) :
  KFM::Application_skel( _obj )
{
}

OpenParts::Part_ptr KfmApplicationIf::createPart()
{
  QString home = "file:";
  home.detach();
  home += QDir::homeDirPath().data();

  return OpenParts::Part::_duplicate( new KfmGui( home.data() ) );
}

OpenParts::MainWindow_ptr KfmApplicationIf::createWindow()
{
  QString home = "file:";
  home.detach();
  home += QDir::homeDirPath().data();
  
  return OpenParts::MainWindow::_duplicate( (new KfmMainWindow( home.data() ))->interface() );
}

/**********************************************
 *
 * KfmApp
 *
 **********************************************/

KfmApp::KfmApp( int &argc, char** argv ) : 
  OPApplication( argc, argv, "konqueror" )
{
}

KfmApp::~KfmApp()
{
}

void KfmApp::start()
{
  if ( g_bWithGUI )
  {
    QString home = "file:";
    home.detach();
    home += QDir::homeDirPath().data();
    KfmMainWindow *m_pShell = new KfmMainWindow( home.data() );
    m_pShell->show();
  }
}


/**********************************************
 *
 * KfmBookmarkManager
 *
 **********************************************/

void KfmBookmarkManager::editBookmarks( const char *_url )
{
  KfmMainWindow *m_pShell = new KfmMainWindow( _url );
  m_pShell->show();
}


/**********************************************
 *
 * MAIN
 *
 **********************************************/

int main( int argc, char **argv )
{
  KfmBoot boot( "IDL:KFM/Application:1.0", "Konqueror" );

  KfmApp app( argc, argv );

  int i = 1;
  if ( strcmp( argv[i], "-s" ) == 0 || strcmp( argv[i], "--server" ) == 0 )
  {
    i++;
    g_bWithGUI = false;
  }
  
  signal(SIGCHLD,sig_handler);
  signal(SIGTERM,sig_term_handler);
  signal(SIGPIPE,sig_pipe_handler);

  ProtocolManager manager;

  KfmPaths::initStatic();
  KIOJob::initStatic();
  KIOCache::initStatic();
  // KMimeType::initStatic();
  // KService::initStatic();
  // KServiceTypeProfile::initStatic();

  KRegistry registry;
  registry.addFactory( new KMimeTypeFactory );
  registry.addFactory( new KServiceFactory );
  // HACK
  registry.load( "/tmp/dump" );
  if ( registry.isModified() )
  {    
    registry.save( "/tmp/dump" );
    registry.clearModified();
  }
  
  KMimeType::check();

  KMimeMagic::initStatic();

  cerr << "===================== fuck you ==============" << endl;
  
  kimgioRegister();
  
  QImageIO::defineIOHandler( "XV", "^P7 332", 0, read_xv_file, 0L );

  KfmBookmarkManager bm;
  
  app.exec();

  cerr << "============ BACK from event loop ===========" << endl;

  return 0;
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

#include "kfm_main.moc"
