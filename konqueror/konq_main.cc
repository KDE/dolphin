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
#include <komBase.h>

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

#include "konq_main.h"
#include "konq_mainwindow.h"
#include "kfmpaths.h"
#include "xview.h"
#include "konq_mainview.h"
#include "konq_htmlview.h"
#include "konq_iconview.h"

// DEBUG
#include <iostream>

void sig_handler( int signum );
void sig_term_handler( int signum );
void sig_pipe_handler( int signum );

bool g_bWithGUI = true;

/**********************************************
 *
 * KonqApplicationIf
 *
 **********************************************/

typedef KOMBoot<KonqApplicationIf> KonqBoot;

KonqApplicationIf::KonqApplicationIf()
{
}

KonqApplicationIf::KonqApplicationIf( const CORBA::BOA::ReferenceData &refdata ) :
  Konqueror::Application_skel( refdata )
{
}

KonqApplicationIf::KonqApplicationIf( CORBA::Object_ptr _obj ) :
  Konqueror::Application_skel( _obj )
{
}

OpenParts::Part_ptr KonqApplicationIf::createPart()
{
  QString home = "file:";
  home.detach();
  home += QDir::homeDirPath().data();

  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( home.data() );
  eventURL.reload = (CORBA::Boolean)true;

  OpenParts::Part_var m_vMainView = OpenParts::Part::_duplicate( new KonqMainView );
  
  EMIT_EVENT( m_vMainView, Konqueror::eventOpenURL, eventURL );  
  
  return OpenParts::Part::_duplicate( m_vMainView );
}

OpenParts::MainWindow_ptr KonqApplicationIf::createWindow()
{
  QString home = "file:";
  home.detach();
  home += QDir::homeDirPath().data();
  
  return OpenParts::MainWindow::_duplicate( (new KonqMainWindow( home.data() ))->interface() );
}

Konqueror::MainView_ptr KonqApplicationIf::createMainView()
{
  QString home = "file:";
  home.detach();
  home += QDir::homeDirPath().data();

  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( home.data() );
  eventURL.reload = (CORBA::Boolean)true;

  Konqueror::MainView_var m_vMainView = Konqueror::MainView::_duplicate( new KonqMainView );
  
  EMIT_EVENT( m_vMainView, Konqueror::eventOpenURL, eventURL );  
  
  return Konqueror::MainView::_duplicate( m_vMainView );
}

Konqueror::KfmIconView_ptr KonqApplicationIf::createKfmIconView()
{
  return Konqueror::KfmIconView::_duplicate( new KonqKfmIconView );
}

Konqueror::HTMLView_ptr KonqApplicationIf::createHTMLView()
{
  return Konqueror::HTMLView::_duplicate( new KonqHTMLView );
}

Konqueror::KfmTreeView_ptr KonqApplicationIf::createKfmTreeView()
{
//  return Konqueror::KfmTreeView::_duplicate( new KonqKfmTreeView );
}

Konqueror::PartView_ptr KonqApplicationIf::createPartView()
{
//  return Konqueror::PartView::_duplicate( new KonqPartView );
}

/**********************************************
 *
 * KonqApp
 *
 **********************************************/

KonqApp::KonqApp( int &argc, char** argv ) : 
  OPApplication( argc, argv, "konqueror" )
{
}

KonqApp::~KonqApp()
{
}

void KonqApp::start()
{
  if ( g_bWithGUI )
  {
    QString home = "file:";
    home.detach();
    home += QDir::homeDirPath().data();
    KonqMainWindow *m_pShell = new KonqMainWindow( home.data() );
    m_pShell->show();
  }
}


/**********************************************
 *
 * KonqBookmarkManager
 *
 **********************************************/

void KonqBookmarkManager::editBookmarks( const char *_url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( _url );
  m_pShell->show();
}


/**********************************************
 *
 * MAIN
 *
 **********************************************/

int main( int argc, char **argv )
{
  KonqBoot boot( "IDL:Konqueror/Application:1.0", "Konqueror" );

  KonqApp app( argc, argv );

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
  registry.load( );
  
  KMimeType::check();

  KMimeMagic::initStatic();

  cerr << "===================== mime stuff finished ==============" << endl;
  
  kimgioRegister();
  
  QImageIO::defineIOHandler( "XV", "^P7 332", 0, read_xv_file, 0L );

  KonqBookmarkManager bm;
  
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

#include "konq_main.moc"
