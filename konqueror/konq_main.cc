/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <qdir.h>

#include <opApplication.h>
#include <komApplication.h>
#include <komBase.h>
#include <komShutdownManager.h>

#include <kservices.h>
#include <kregistry.h>
#include <kregfactories.h>
#include <kio_cache.h>

#include <kded_instance.h>
#include <ktrader.h> //for KTraderServiceProvider
#include <knaming.h>

#include <kimgio.h>
#include <kapp.h>
#include <klocale.h>
#include <kglobal.h>
#include <kwm.h>
#include <userpaths.h>

#include <qmessagebox.h>

#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include "kfmrun.h"
#include "konq_main.h"
#include "konq_mainwindow.h"
#include "konq_topwidget.h"
#include "konq_mainview.h"
#include "konq_iconview.h"
#include "konq_htmlview.h"
#include "konq_partview.h"
#include "konq_treeview.h"
#include "konq_txtview.h"
#include <kstddirs.h>

void sig_handler( int signum );
void sig_term_handler( int signum );
void sig_pipe_handler( int signum );

bool g_bWithGUI = true;

bool g_bUseNaming = false;

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

KonqMainView *KonqApplicationIf::allocMainView()
{
  KonqMainView *mainView = new KonqMainView;
  KOMShutdownManager::self()->watchObject( mainView );
  return mainView;
}

KonqMainWindow *KonqApplicationIf::allocMainWindow( const char *url )
{
  KonqMainWindow * mw = new KonqMainWindow( url );
  mw->show(); // ahah, it won't show up, otherwise !
  KOMShutdownManager::self()->watchObject( mw->interface() );
  return mw;
}

OpenParts::Part_ptr KonqApplicationIf::createPart()
{
  return OpenParts::Part::_duplicate( allocMainView() );
}

OpenParts::MainWindow_ptr KonqApplicationIf::createWindow()
{
  return OpenParts::MainWindow::_duplicate( allocMainWindow()->interface() );
}

Konqueror::MainWindow_ptr KonqApplicationIf::createMainWindow( const char* url )
{
  return Konqueror::MainWindow::_duplicate( allocMainWindow( url )->konqInterface() );
}

Konqueror::MainView_ptr KonqApplicationIf::createMainView()
{
  return Konqueror::MainView::_duplicate( allocMainView() );
}

Konqueror::KfmIconView_ptr KonqApplicationIf::createKfmIconView()
{
  KonqKfmIconView *iconView = new KonqKfmIconView;
  KOMShutdownManager::self()->watchObject( iconView );
  return Konqueror::KfmIconView::_duplicate( iconView );
}

Konqueror::HTMLView_ptr KonqApplicationIf::createHTMLView()
{
  KonqHTMLView *htmlView = new KonqHTMLView;
  KOMShutdownManager::self()->watchObject( htmlView );
  return Konqueror::HTMLView::_duplicate( htmlView );
}

Konqueror::KfmTreeView_ptr KonqApplicationIf::createKfmTreeView()
{
  KonqKfmTreeView *treeView = new KonqKfmTreeView;
  KOMShutdownManager::self()->watchObject( treeView );
  return Konqueror::KfmTreeView::_duplicate( treeView );
}

Konqueror::PartView_ptr KonqApplicationIf::createPartView()
{
  KonqPartView *partView = new KonqPartView;
  KOMShutdownManager::self()->watchObject( partView );
  return Konqueror::PartView::_duplicate( partView );
}

Konqueror::TxtView_ptr KonqApplicationIf::createTxtView()
{
  KonqTxtView *txtView = new KonqTxtView;
  KOMShutdownManager::self()->watchObject( txtView );
  return Konqueror::TxtView::_duplicate( txtView );
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
    (void) new KonqTopWidget();
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

/************* Functions called by main ****************/

void testDir( const char *_name, bool showMsg = FALSE )
{
  DIR *dp;
  dp = opendir( _name );
  if ( dp == NULL )
  {
    QString m = _name;
    if ( m.right(1) == "/" )
      m.truncate( m.length() - 1 );
    
    if (showMsg)
      QMessageBox::information( 0, i18n("Information"),
                                i18n("Creating directory:\n") + m,
				i18n("OK") );
    ::mkdir( m, S_IRWXU );
  }
  else
    closedir( dp );
}
 
void copyDirectoryFile(const char *filename, const QString& dir, bool force)
{
  if (force || !QFile::exists(dir + "/.directory")) {
    QString cmd;
    cmd.sprintf( "cp %s/konqueror/%s %s/.directory", kapp->kde_datadir().data(),
                 filename, dir.data() );
    system( cmd.data() );
  }
}

/**
 * @return true if this is the first time 
 * konqy is run for the current release
 * WARNING : call only once !!! (second call will always return false !)
 */
bool isNewRelease()
{
    bool bNewRelease = false;
 
    KConfig* config = kapp->getConfig();
    config->setGroup("Version");
    int versionMajor = config->readNumEntry("KDEVersionMajor", 0);
    int versionMinor = config->readNumEntry("KDEVersionMinor", 0);
    int versionRelease = config->readNumEntry("KDEVersionRelease", 0);
 
    if( versionMajor < KDE_VERSION_MAJOR )
        bNewRelease = true;
    else if( versionMinor < KDE_VERSION_MINOR )
             bNewRelease = true;
         else if( versionRelease < KDE_VERSION_RELEASE )
                  bNewRelease = true;
 
    if( bNewRelease ) {
      config->writeEntry("KDEVersionMajor", KDE_VERSION_MAJOR );
      config->writeEntry("KDEVersionMinor", KDE_VERSION_MINOR );
      config->writeEntry("KDEVersionRelease", KDE_VERSION_RELEASE );
      config->sync();
    }

    return bNewRelease;
}

/*
 * @return true if we must copy Templates, i.e. if new release and if the user agrees.
 * or if there are simply no Templates installed at all
 */
bool checkTemplates( bool bNewRelease )
{
    // Test for existing Templates
    bool bTemplates = true;
 
    DIR* dp = 0L; // avoid warning
    dp = opendir( UserPaths::templatesPath() );
    if ( dp == 0L )
      bTemplates = false;
    else
      closedir( dp );
 
    if( bNewRelease && bTemplates )
    {
      int btn = QMessageBox::information( 0,
                i18n("Information"),
                i18n("A new KDE version has been installed.\nThe Template files may have changed.\n\nWould you like to install the new ones?"),
                i18n("Yes"), i18n("No") );
      if( !btn ) { // "Yes"
        bTemplates = false; // force reinstall
      }
    }
    return !bTemplates; // No templates installed, or update wanted
}

/*
 * Create, if necessary, some dirs and some .directory in user's .kde/
 * and copy Templates, still if necessary only
 */
void testLocalInstallation()
{
  KGlobal::dirs()->getSaveLocation("data", "konqueror", true); // for kfmclient

  bool newRelease = isNewRelease();
  bool copyTemplates = checkTemplates(newRelease);

  testDir( UserPaths::desktopPath(), TRUE );
  copyDirectoryFile("directory.desktop", UserPaths::desktopPath(), false);
  testDir( UserPaths::trashPath() );
  copyDirectoryFile("directory.trash", UserPaths::trashPath(), newRelease); //for trash icon
  testDir( UserPaths::templatesPath() );
  copyDirectoryFile("directory.templates", UserPaths::templatesPath(), copyTemplates);
  testDir( UserPaths::autostartPath() );
  copyDirectoryFile("directory.autostart", UserPaths::autostartPath(), false);

  if (copyTemplates)
  {
    QString cmd;
    cmd.sprintf("cp %s/konqueror/Templates/* %s",
                kapp->kde_datadir().data(),
                UserPaths::templatesPath().data() );
    system( cmd.data() );
    KWM::sendKWMCommand("krootwm:refreshNew");
  }
}

/**********************************************
 *
 * MAIN
 *
 **********************************************/

int main( int argc, char **argv )
{
  // kfm uses lots of colors, especially when browsing the web
  // The call below helps for 256-color displays
  // Kudos to Nikita V. Youshchenko !
  QApplication::setColorSpec( QApplication::ManyColor ); 

  KonqApp app( argc, argv );
  
  // create a KdedInstance object, which takes care about connecting to kded
  // and providing a trader and an activator
  KdedInstance kded( argc, argv, komapp_orb );
  
  KonqBoot boot( "IDL:Konqueror/Application:1.0", "App" );
  
  KGlobal::locale()->insertCatalogue("libkonq"); // needed for apps using libkonq

  int i = 1;
  if ( strcmp( argv[i], "-s" ) == 0 || strcmp( argv[i], "--server" ) == 0 )
  {
    i++;
    g_bWithGUI = false;
  }

  signal(SIGCHLD,sig_handler);
  signal(SIGTERM,sig_term_handler);
  signal(SIGPIPE,sig_pipe_handler);

  KIOCache::initStatic();
  
  KonqFileManager fm;
  KTraderServiceProvider sp;

  testLocalInstallation();
  
  KRegistry registry;
  registry.addFactory( new KServiceTypeFactory );
  // registry.addFactory( new KServiceFactory ); // not needed anymore - we use kded
  registry.load( );

  kimgioRegister();

  QString path = KGlobal::dirs()->getSaveLocation("data", "kfm/bookmarks", true);
  KonqBookmarkManager bm ( path );

  KNaming *naming = kded.knaming();
  CORBA::Object_var obj = naming->resolve( Konqueror::KONQ_NAMING );
  if ( CORBA::is_nil( obj ) )
  {
    KonqApplicationIf *appIf = new KonqApplicationIf;
    appIf->incRef(); //prevent anyone from destroying us
    g_bUseNaming = naming->bind( Konqueror::KONQ_NAMING, appIf );
  }

  app.exec();

  kdebug(0, 1202, "============ BACK from event loop ===========");
  sig_term_handler(0);

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
  kdebug( KDEBUG_INFO, 1202, "###################### SIG TERM ###################" );

  if ( KonqTopWidget::isGoingDown() )
    return;

  KonqTopWidget::getKonqTopWidget()->slotSave();
  KonqTopWidget::getKonqTopWidget()->slotShutDown();

  exit(1);
}

void sig_pipe_handler( int )
{
  kdebug( KDEBUG_INFO, 1202, "###################### SIG PIPE ###################" );
  signal( SIGPIPE, sig_pipe_handler );
}

#include "konq_main.moc"
