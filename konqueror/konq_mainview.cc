/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#include <kparts/browserextension.h>
#include "konq_htmlsettings.h"
#include "konq_mainview.h"
#include "konq_childview.h"
#include "konq_run.h"
#include "konq_misc.h"
#include "konq_factory.h"
#include "konq_viewmgr.h"
#include "konq_frame.h"
#include "konq_events.h"
#include "konq_actions.h"

#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qmetaobject.h>
#include <qlayout.h>

#include <kaction.h>
#include <kdebug.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <knewmenu.h>
#include <kstdaccel.h>
#include <kstddirs.h>
#include <ksycoca.h>
#include <kurl.h>
#include <kwm.h>
#include <klineeditdlg.h>
#include <konqdefaults.h>
#include <konqpopupmenu.h>
#include <konqsettings.h>
#include <kprogress.h>
#include <kio/job.h>
#include <ktrader.h>
#include <kapp.h>
#include <dcopclient.h>
#include <klibloader.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
#include <kbookmarkbar.h>
#include <kstdaction.h>
#include <khelpmenu.h>
#include <kparts/part.h>
#include <kmenubar.h>
#include <kglobalsettings.h>
#include <kedittoolbar.h>

#include "version.h"

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

template class QList<QPixmap>;
template class QList<KToggleAction>;

QList<QPixmap> *KonqMainView::s_plstAnimatedLogo = 0L;
bool KonqMainView::s_bMoveSelection = false;

KonqMainView::KonqMainView( const KURL &initialURL, bool openInitialURL, const char *name )
 : KParts::MainWindow( name ),  DCOPObject( "KonqMainViewIface" )
{
  m_currentView = 0L;
  m_pBookmarkMenu = 0L;
  m_bURLEnterLock = false;

  KonqFactory::instanceRef();

  if ( !s_plstAnimatedLogo )
    s_plstAnimatedLogo = new QList<QPixmap>;

  if ( s_plstAnimatedLogo->count() == 0 )
  {
    s_plstAnimatedLogo->setAutoDelete( true );
    for ( int i = 1; i < 9; i++ )
    {
      QString n;
      n.sprintf( "kde%i", i );
      s_plstAnimatedLogo->append( new QPixmap( BarIcon( n, KonqFactory::instance() ) ) );
    }
  }

  m_animatedLogoCounter = 0;
  connect( &m_animatedLogoTimer, SIGNAL( timeout() ),
           this, SLOT( slotAnimatedLogoTimeout() ) );

  m_pViewManager = new KonqViewManager( this );

  connect( m_pViewManager, SIGNAL( activePartChanged( KParts::Part * ) ),
	   this, SLOT( slotPartActivated( KParts::Part * ) ) );

  m_viewModeGUIClient = new ViewModeGUIClient( this );

  initActions();
  initPlugins();

  setInstance( KonqFactory::instance() );

  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

  setXMLFile( locate( "data", "konqueror/konqueror.rc" ) );

  createGUI( 0L );

  // hide if empty
  KToolBar * bar = (KToolBar *)child( "bookmarkToolBar", "KToolBar" );
  if ( bar && bar->count() <= 1 ) // there is always a separator
  {
    m_paShowBookmarkBar->setChecked( false );
    bar->hide();
  }

/*
  m_statusBar = statusBar();

  m_progressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, m_statusBar );

  m_statusBar->insertWidget( m_progressBar, 120, STATUSBAR_LOAD_ID );
  m_statusBar->insertItem( QString::fromLatin1( "XXXXXXXX" ), STATUSBAR_SPEED_ID );
  m_statusBar->insertItem( 0L, STATUSBAR_MSG_ID );

  m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );

  m_statusBar->show();

  m_progressBar->hide();
*/

  if ( !initialURL.isEmpty() )
    openFilteredURL( 0L, initialURL.url() );
  else if ( openInitialURL )
  {
    KConfig *config = KonqFactory::instance()->config();

    if ( config->hasGroup( "Default View Profile" ) )
    {
      config->setGroup( "Default View Profile" );
      enableAllActions( true );
      m_pViewManager->loadViewProfile( *config );
    }
    else
      openURL( 0L, KURL( QDir::homeDirPath().prepend( "file:" ) ) );
  }

  resize( 700, 480 );
  m_bFullScreen = false;
}

KonqMainView::~KonqMainView()
{
  if ( m_combo )
  {

    QStringList comboItems;

    for ( int i = 0; i < m_combo->count(); i++ )
      comboItems.append( m_combo->text( i ) );

    while ( comboItems.count() > 10 )
      comboItems.remove( comboItems.fromLast() );

    KConfig *config = KonqFactory::instance()->config();
    config->setGroup( "Settings" );
    config->writeEntry( "ToolBarCombo", comboItems );
    config->sync();
  }

  m_animatedLogoTimer.stop();
  delete m_pViewManager;

  if ( m_pBookmarkMenu )
    delete m_pBookmarkMenu;

  KonqFactory::instanceUnref();
}

QWidget *KonqMainView::createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, containerStateBuffer, id );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( res->inherits( "KToolBar" ) );

    (void)new KBookmarkBar( this, (KToolBar *)res, actionCollection(), this );
  }

  return res;
}

QString KonqMainView::konqFilteredURL( const QString &_url )
{
  QString url = _url.stripWhiteSpace();

  // Absolute path?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
    url.prepend( "file:" );
  }
  // Home directory?
  else if ( url.find( QRegExp( "^~.*" ) ) == 0 )
  {
    QString user;
    QString path;
    int index;

    index = url.find( "/" );
    if ( index != -1 )
    {
      user = url.mid( 1, index-1 );
      path = url.mid( index+1 );
    }
    else
      user = url.mid( 1 );

    if ( user.isEmpty() )
      user = QDir::homeDirPath();
    else
    {
      struct passwd *pwe = getpwnam( user.latin1() );
      if ( !pwe )
      {
	KMessageBox::sorry( this, i18n( "User %1 doesn't exist" ).arg( user ));
	return QString::null;
      }
      user = QString::fromLatin1( pwe->pw_dir );
    }

    KURL u( user + '/' + path );
    url = u.url();
  }
  else if ( strncmp( url, "www.", 4 ) == 0 )
  {
    QString tmp = "http://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }
  else if ( strncmp( url, "ftp.", 4 ) == 0 )
  {
    QString tmp = "ftp://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }
  // Samba notation for \\host\share?
  else if ( strncmp( url, "\\\\", 2 ) == 0 )
  {
    for (unsigned int i=0; i<url.length(); i++ )
      if (url[i] == '\\') url[i]='/';
    KURL::encode( url );
    url.prepend( "smb:" );
  }
/*
  else if ( gethostbyname( url.ascii() ) != 0 )
  {
    QString tmp = "http://";
    KURL::encode( url );
    tmp += url;
    tmp += "/";
    url = tmp;
  }
*/

  return url;
}

void KonqMainView::openFilteredURL( KonqChildView * /*_view*/, const QString &_url )
{
  KonqURLEnterEvent ev( konqFilteredURL( _url ) );
  QApplication::sendEvent( this, &ev );
}

void KonqMainView::openURL( KonqChildView *_view, const KURL &url, bool reload, int xOffset,
                            int yOffset, const QString &serviceType )
{
  kdDebug(1202) << "KonqMainView::openURL : _url = '" << url.url() << "'\n";

  if ( url.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(url.url());
    KMessageBox::error(0, tmp);
    return;
  }

  KonqChildView *view = _view;
  if ( !view || view->passiveMode() )
    view = (KonqChildView *)m_currentView;

  if ( view )
  {
    if ( view == (KonqChildView *)m_currentView )
      //will do all the stuff below plus GUI stuff
      slotStop();
    else
      view->stop();

    setLocationBarURL( view, url.decodedURL() );
    view->setMiscURLData( reload, xOffset, yOffset );
  }
  else
  {
    if ( m_combo )
      m_combo->setEditText( url.decodedURL() );
  }

  kDebugInfo( 1202, "%s", QString("trying openView for %1 (servicetype %2)").arg(url.url()).arg(serviceType).latin1() );
  if ( !serviceType.isEmpty() )
  {
    if ( !openView( serviceType, url, view /* can be 0L */) )
    {
      kDebugInfo( 1202, "%s", QString("Creating new KRun for %1").arg(url.url()).latin1() );
      (void)new KRun( url );
    }
  }
  else
  {
    kDebugInfo( 1202, "%s", QString("Creating new konqrun for %1").arg(url.url()).latin1() );
    KonqRun * run = new KonqRun( this, view /* can be 0L */, url, 0, false, true );
    if ( view )
      view->setRun( run );
    connect( run, SIGNAL( finished() ),
             this, SLOT( slotRunFinished() ) );
    connect( run, SIGNAL( error() ),
             this, SLOT( slotRunFinished() ) );
  }

  if ( view && view == m_currentView && serviceType.isEmpty() )
  {
    view->setLoading( true );
    startAnimation();
  }
}

void KonqMainView::openURL( const KURL &url, bool reload, int xOffset,
                            int yOffset, const QString &serviceType )
{
  KonqChildView *childV = 0L;

  if ( sender() )
    childV = childView( (KParts::ReadOnlyPart *)sender()->parent() );

  openURL( childV, url, reload, xOffset, yOffset, serviceType );
}

void KonqMainView::openURL( const KURL &url, const KParts::URLArgs &args )
{
  //TODO: handle post data!

  //  ### HACK !!
  if ( args.postData.size() > 0 )
  {
    KonqChildView *view = childView( (KParts::ReadOnlyPart *)sender()->parent() );
    view->browserExtension()->setURLArgs( args );
    openURL( view, url, args.reload, args.xOffset, args.yOffset, QString::fromLatin1( "text/html" ) );
    return;
  }

  openURL( url, args.reload, args.xOffset, args.yOffset, args.serviceType );
}

void KonqMainView::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args )
{
  //FIXME: obey args (like passing post-data (to KRun), etc.)
  KonqFileManager::getFileManager()->openFileManagerWindow( url.url() );
}

void KonqMainView::slotNewWindow()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( m_currentView->view()->url().url() );
}

void KonqMainView::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. Minicli does not support that yet
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "popupExecuteCommand()", data );
}

void KonqMainView::slotOpenTerminal()
{
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Misc Defaults" );
  QString term = config->readEntry( "Terminal", DEFAULT_TERMINAL );

  QString dir ( QDir::homeDirPath() );

  KURL u( m_currentView->url() );
  if ( u.isLocalFile() )
    dir = u.path();

  QString cmd = QString("cd \"%1\" ; %2 &").arg( dir ).arg( term );
  system( cmd.latin1() );
}

void KonqMainView::slotOpenLocation()
{
  QString u;

  if (m_currentView)
    u = m_currentView->url().url();

  KLineEditDlg l( i18n("Open Location:"), u, this, true );
  int x = l.exec();
  if ( x )
  {
    u = l.text();
    u = u.stripWhiteSpace();
    // Exit if the user did not enter an URL
    if ( u.isEmpty() )
      return;

    openFilteredURL( 0L, u );
  }
}

void KonqMainView::slotToolFind()
{
  KShellProcess proc;
  proc << "kfind";

  if ( m_currentView ) // play safe
  {
    KURL url;
    url = m_currentView->url();

    if( url.isLocalFile() )
    {
      if ( m_currentView->serviceType() == "inode/directory" )
        proc << url.path();
      else
        proc << url.directory();
    }
  }

  proc.start(KShellProcess::DontCare);
}

void KonqMainView::slotViewModeToggle( bool toggle )
{
  if ( !toggle )
    return;

  QString modeName = sender()->name();

  if ( m_currentView->service()->name() == modeName )
    return;

  m_currentView->changeViewMode( m_currentView->serviceType(), m_currentView->url(),
                                 false, modeName );
}

void KonqMainView::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

  m_currentView->setAllowHTML( b );

  if ( b && m_currentView->supportsServiceType( "inode/directory" ) )
  {
    m_currentView->lockHistory();
    openURL( 0L, m_currentView->url() );
  }
  else if ( !b && m_currentView->supportsServiceType( "text/html" ) )
  {
    KURL u( m_currentView->url() );
    m_currentView->lockHistory();
    openURL( 0L, u.directory() );
  }
}

void KonqMainView::slotToggleDirTree( bool toggle )
{
  KonqFrameContainer *mainContainer = m_pViewManager->mainContainer();

  assert( mainContainer );

  if ( toggle )
  {
    KonqFrameBase *splitFrame = mainContainer->firstChild();

    KonqFrameContainer *newContainer;

    KParts::ReadOnlyPart *view = m_pViewManager->split( splitFrame, Qt::Horizontal, QString::fromLatin1( "Browser/View" ), QString::fromLatin1( "KonqDirTree" ), &newContainer );

    newContainer->moveToLast( splitFrame->widget() );

    KonqFrameBase *firstCh = newContainer->firstChild();
    KonqFrameBase *secondCh = newContainer->secondChild();
    newContainer->setFirstChild( secondCh );
    newContainer->setSecondChild( firstCh );

    QValueList<int> newSplitterSizes;
    newSplitterSizes << 30 << 100;

    newContainer->setSizes( newSplitterSizes );

    KonqChildView *cv = childView( view );

    cv->setPassiveMode( true );
    cv->frame()->statusbar()->passiveModeCheckBox()->setChecked( true );
  }
  else
  {
    QList<KonqChildView> viewList;

    mainContainer->listViews( &viewList );

    QListIterator<KonqChildView> it( viewList );

    for (; it.current(); ++it )
      if ( it.current()->view()->inherits( "KonqDirTreePart" ) )
        m_pViewManager->removeView( it.current() );
  }
}

void KonqMainView::slotStop()
{
  if ( m_currentView )
  {
    m_currentView->stop(); // will take care of the statusbar
    stopAnimation();
  }
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotUp()
{
  openURL( (KonqChildView *)m_currentView, m_currentView->url().upURL(true) );
}

void KonqMainView::slotBack()
{
  m_currentView->go( -1 );
}

void KonqMainView::slotForward()
{
  m_currentView->go( 1 );
}

void KonqMainView::slotHome()
{
  openURL( 0L, KURL( konqFilteredURL( KonqFMSettings::defaultIconSettings()->homeURL() ) ) );
}

void KonqMainView::slotGoMimeTypes()
{
  openURL( m_currentView, KURL( KonqFactory::instance()->dirs()->saveLocation("mime") ) );
}

void KonqMainView::slotGoApplications()
{
  openURL( m_currentView, KURL( KonqFactory::instance()->dirs()->saveLocation("apps") ) );
}

void KonqMainView::slotGoDirTree()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( locateLocal( "data", "konqueror/dirtree/" ) );
}

void KonqMainView::slotGoTrash()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( KGlobalSettings::trashPath() );
}

void KonqMainView::slotSaveSettings()
{
  // "savePropertiesAsDefault()" is automatically called in the view
  // But we also connected this slot here, in order to
  // save here konqueror settings (show menubar, ...)
  // (TODO)
}

void KonqMainView::slotConfigureFileManager()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "Applications/kcmkonq", 0);
    warning("Error launching kcmshell Applications/konqueror!");
    exit(1);
  }
}

void KonqMainView::slotConfigureBrowser()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "Applications/konqhtml", 0);
    warning("Error launching kcmshell Applications/konqhtml!");
    exit(1);
  }
}

void KonqMainView::slotConfigureFileTypes()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "filetypes", 0);
    warning("Error launching kcmshell filetypes !");
    exit(1);
  }
}

void KonqMainView::slotConfigureNetwork()
{
  // This should be kcmshell instead, when it supports multiple tabs
  const char * cmd = "kcontrol Network/smb Network/cookies Network/useragent Network/proxy";
  system( cmd );
}

void KonqMainView::slotConfigureKeys()
{
  //just testing Torben's widget :-) -- to be replaced by kdelibs config widget (Simon)
  //QActionDialog *actionDia = new QActionDialog( actionCollection(), this, 0 );
  //actionDia->show();
}

void KonqMainView::slotConfigureToolbars()
{
  QActionCollection collection;
  collection = *actionCollection() + *currentView()->actionCollection();
  KEditToolbar edit(&collection, xmlFile(), currentView()->xmlFile());
  if (edit.exec())
  {
    // don't know how to recreate GUI
  }
}

void KonqMainView::slotViewChanged( KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView )
{
  m_mapViews.remove( oldView );
  m_mapViews.insert( newView, (KonqChildView *)sender() );

  if ( (KonqChildView *)sender() == (KonqChildView *)m_currentView )
  {
    m_pViewManager->removePart( oldView );
    updateStatusBar();
  }

  // Add the new part to the manager
  m_pViewManager->addPart( newView, true );
}


void KonqMainView::slotRunFinished()
{
  KonqRun *run = (KonqRun *)sender();

  if ( run->foundMimeType() )
    return;

  KonqChildView *childView = run->childView();

  if ( run->hasError() )
  {
    kDebugWarning( 1202, " Couldn't run ... what do we do ? " );
    if ( !childView ) // Nothing to show ??
    {
      close(); // This window is useless
      KMessageBox::sorry(0L, i18n("Could not display the requested URL, closing the window"));
      // or should we go back $HOME ?
      return;
    }
  }

  if ( !childView )
    return;

  childView->setLoading( false );

  if ( childView == m_currentView )
  {
    stopAnimation();
    setLocationBarURL( childView, childView->view()->url().url() );
  }
}

void KonqMainView::slotSetStatusBarText( const QString & )
{
   // Reimplemented to disable KParts::MainWindow default behaviour
   // Does nothing here, see konq_frame.cc
}

bool KonqMainView::openView( QString serviceType, const KURL &_url, KonqChildView *childView )
{
  kDebugInfo( 1202, " KonqMainView::openView %s %s", serviceType.ascii(), _url.url().ascii());
  QString indexFile;

  KURL url = _url;

  if ( !childView )
    {
      KParts::ReadOnlyPart* view = m_pViewManager->splitView( Qt::Horizontal, url, serviceType );

      if ( !view )
      {
        KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nCheck your installation").arg(serviceType) );
        return true; // fake everything was ok, we don't want to propagate the error
      }

      enableAllActions( true ); // can't we rely on setActiveView to do the right thing ? (David)

      m_pViewManager->setActivePart( view );

      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );

      view->widget()->setFocus();

      return true;
    }
  else
  {
    kDebugInfo( 1202, "%s", QString("(1) KonqMainView::openView : url = '%1'").arg(url.url()).latin1());
    // This is already called in ::openURL
    //    childView->stop();
  }
  kDebugInfo( 1202, "%s", QString("(2) KonqMainView::openView : url = '%1'").arg(url.url()).latin1());

  if ( ( serviceType == "inode/directory" ) &&
       ( childView->allowHTML() ) &&
         ( url.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( url.path() ) ) != QString::null ) )
  {
    serviceType = "text/html";
    url = KURL( indexFile.prepend( "file:" ) );
  }

  if ( childView->changeViewMode( serviceType, url ) )
  {
    // Give focus to the view
    childView->view()->widget()->setFocus();
    return true;
  }

  // Didn't work, abort
  childView->setLoading( false );
  if ( childView == m_currentView )
  {
    stopAnimation();
    setLocationBarURL( childView, childView->view()->url().url() );
  }

  return false;
}

void KonqMainView::slotPartActivated( KParts::Part *part )
{
  if ( !part )
  {
    createGUI( 0L );
    return;
  }

  KonqChildView *newView = m_mapViews.find( (KParts::ReadOnlyPart *)part ).data();

  if ( newView->passiveMode() )
  {
    if ( newView->browserExtension() )
      connectExtension( newView->browserExtension() );
    return;
  }

  KonqChildView *oldView = m_currentView;

  if ( m_currentView && m_currentView->browserExtension() )
    disconnectExtension( m_currentView->browserExtension() );

  m_currentView = newView;

  if ( m_currentView->browserExtension() )
  {
    connectExtension( m_currentView->browserExtension() );
    createGUI( part );
  }
  else
  {
    //we should find some central place for those actions.... Perhaps in kparts,
    // as a string array? (Simon)
    m_paCut->setEnabled( false );
    m_paCopy->setEnabled( false );
    m_paPaste->setEnabled( false );
    m_paDelete->setEnabled( false );
    m_paTrash->setEnabled( false );
    m_paShred->setEnabled( false );
    m_paPrint->setEnabled( false );

    createGUI( 0L );
  }

  guiFactory()->removeClient( m_viewModeGUIClient );
  m_viewModeGUIClient->update( m_currentView->serviceOffers() );
  if ( m_currentView->serviceOffers().count() > 1 )
    guiFactory()->addClient( m_viewModeGUIClient );

  m_currentView->frame()->statusbar()->repaint();

  if ( oldView )
    oldView->frame()->statusbar()->repaint();

  if ( m_combo )
    m_combo->setEditText( m_currentView->locationBarURL() );

  updateStatusBar();
  updateToolBarActions();
}

void KonqMainView::insertChildView( KonqChildView *childView )
{
  m_mapViews.insert( childView->view(), childView );

  m_paRemoveView->setEnabled( m_mapViews.count() > 1 );

  if ( childView->view()->inherits( "KonqDirTreeBrowserView" ) )
  {
    m_ptaShowDirTree->blockSignals( true );
    m_ptaShowDirTree->setChecked( true );
    m_ptaShowDirTree->blockSignals( false );
  }
}

void KonqMainView::removeChildView( KonqChildView *childView )
{
  MapViews::Iterator it = m_mapViews.find( childView->view() );
  m_mapViews.remove( it );

  m_paRemoveView->setEnabled( m_mapViews.count() > 1 );

  if ( childView == m_currentView )
  {
    m_currentView = 0L;
    m_pViewManager->setActivePart( 0L );
  }

  bool haveTree = false;
  MapViews::ConstIterator cIt = m_mapViews.begin();
  MapViews::ConstIterator cEnd = m_mapViews.end();
  for (; cIt != cEnd; ++cIt )
    if ( (*cIt)->view()->inherits( "KonqDirTreeBrowserView" ) )
    {
      haveTree = true;
      break;
    }

  if ( !haveTree )
  {
    m_ptaShowDirTree->blockSignals( true );
    m_ptaShowDirTree->setChecked( false );
    m_ptaShowDirTree->blockSignals( false );
  }
}

KonqChildView *KonqMainView::childView( KParts::ReadOnlyPart *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

QValueList<KParts::ReadOnlyPart *> KonqMainView::viewList()
{
  QValueList<KParts::ReadOnlyPart *> res;

  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    res.append( (*it)->view() );

  return res;
}

KParts::ReadOnlyPart *KonqMainView::currentView()
{
  if ( m_currentView )
    return m_currentView->view();
  else
    return 0L;
}

void KonqMainView::customEvent( QCustomEvent *event )
{
  KParts::MainWindow::customEvent( event );

  if ( KonqURLEnterEvent::test( event ) )
  {
    QString url = ((KonqURLEnterEvent *)event)->url();

    openURL( 0L, KURL( url ) );

    return;
  }
}

void KonqMainView::slotAnimatedLogoTimeout()
{
  m_animatedLogoCounter++;
  if ( m_animatedLogoCounter == s_plstAnimatedLogo->count() )
    m_animatedLogoCounter = 0;

  m_paAnimatedLogo->setIconSet( QIconSet( *( s_plstAnimatedLogo->at( m_animatedLogoCounter ) ) ) );
}

void KonqMainView::slotURLEntered( const QString &text )
{
  if ( m_bURLEnterLock )
    return;

  m_bURLEnterLock = true;

  openFilteredURL( 0L, text );

  m_bURLEnterLock = false;
}

void KonqMainView::slotFileNewAboutToShow()
{
  // As requested by KNewMenu :
  m_pMenuNew->slotCheckUpToDate();
  // And set the files that the menu apply on :
  m_pMenuNew->setPopupFiles( m_currentView->url().url() );
}

void KonqMainView::slotSplitViewHorizontal()
{
  m_pViewManager->splitView( Qt::Horizontal );
}

void KonqMainView::slotSplitViewVertical()
{
  m_pViewManager->splitView( Qt::Vertical );
}

void KonqMainView::slotSplitWindowHorizontal()
{
  m_pViewManager->splitWindow( Qt::Horizontal );
}

void KonqMainView::slotSplitWindowVertical()
{
  m_pViewManager->splitWindow( Qt::Vertical );
}

void KonqMainView::slotRemoveView()
{
  KonqChildView *nextView = m_pViewManager->chooseNextView( (KonqChildView *)m_currentView );
  KonqChildView *prevView = m_currentView;

  // Don't remove the last active view
  if ( nextView == 0L )
  	return;
	
  m_pViewManager->setActivePart( nextView->view() );

  m_pViewManager->removeView( prevView );

  if ( m_pViewManager->chooseNextView((KonqChildView *)m_currentView) == 0L )
    m_currentView->frame()->statusbar()->passiveModeCheckBox()->hide();
}

void KonqMainView::slotSaveDefaultProfile()
{
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Default View Profile" );
  m_pViewManager->saveViewProfile( *config );
}

void KonqMainView::speedProgress( int bytesPerSecond )
{
  // We assume this was called from the current view (see KonqChildView::slotSpeedProgress)
/*
  if ( !m_currentView->isLoading() )
    return;

  QString sizeStr;

  if ( bytesPerSecond > 0 )
    sizeStr = KIO::convertSize( bytesPerSecond ) + QString::fromLatin1( "/s" );
  else
    sizeStr = i18n( "stalled" );

  assert(m_statusBar);
  m_statusBar->changeItem( sizeStr, STATUSBAR_SPEED_ID );
*/
}

void KonqMainView::callExtensionMethod( KonqChildView * childView, const char * methodName )
{
  QObject *obj = childView->view()->child( 0L, "KParts::BrowserExtension" );
  assert(obj);
  /*if ( !obj )
    return;*/

  QMetaData * mdata = obj->metaObject()->slot( methodName );
  if( mdata )
    (obj->*(mdata->ptr))();
}

void KonqMainView::slotCut()
{
  // Call cut on the child object
  callExtensionMethod( m_currentView, "cut()" );

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)true;
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = true;
}

void KonqMainView::slotCopy()
{
  // Call copy on the child object
  callExtensionMethod( m_currentView, "copy()" );

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)false;
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = false;
}

void KonqMainView::slotPaste()
{
  if ( s_bMoveSelection )
    callExtensionMethod( m_currentView, "pastecut()" );
  else
    callExtensionMethod( m_currentView, "pastecopy()" );
}

void KonqMainView::slotTrash()
{
  callExtensionMethod( m_currentView, "trash()" );
}

void KonqMainView::slotDelete()
{
  callExtensionMethod( m_currentView, "del()" );
}

void KonqMainView::slotShred()
{
  callExtensionMethod( m_currentView, "shred()" );
}

void KonqMainView::slotSetLocationBarURL( const QString &url )
{
  KParts::ReadOnlyPart *view = (KParts::ReadOnlyPart *)sender()->parent();
  setLocationBarURL( childView( view ), url );
}

void KonqMainView::slotPrint()
{
  // Call print on the child object
  callExtensionMethod( m_currentView, "print()" );
}

void KonqMainView::slotAbout()
{
  KMessageBox::about( 0, i18n(
"Konqueror Version " KONQUEROR_VERSION "\n"
"Author: Torben Weis <weis@kde.org>\n"
"Current maintainer: David Faure <faure@kde.org>\n\n"
"Current team:\n"
"  David Faure <faure@kde.org>\n"
"  Simon Hausmann <hausmann@kde.org>\n"
"  Michael Reiher <michael.reiher@gmx.de>\n"
"  Matthias Welk <welk@fokus.gmd.de>\n"
"HTML rendering engine:\n"
"  Lars Knoll <knoll@kde.org>\n"
"  Antti Koivisto <koivisto@kde.org>\n"
"  Waldo Bastian <bastian@kde.org>\n"
"KIO library/slaves:\n"
"  Matt Koss <koss@napri.sk>\n"
"  Alex Zepeda <garbanzo@hooked.net>\n"
  ));
}

void KonqMainView::slotUpAboutToShow()
{
  QPopupMenu *popup = m_paUp->popupMenu();

  popup->clear();

  uint i = 0;

  KURL u( m_currentView->view()->url() );
  u.cd( ".." );
  while ( u.hasPath() )
  {
    popup->insertItem( KMimeType::pixmapForURL( u ), u.decodedURL() );

    if ( u.path() == "/" )
      break;

    if ( ++i > 10 )
      break;

    u.cd( ".." );
  }
}

void KonqMainView::slotBackAboutToShow()
{
  m_paBack->popupMenu()->clear();
  m_paBack->fillHistoryPopup( m_currentView->history(), 0L, true, false );
}

void KonqMainView::slotForwardAboutToShow()
{
  m_paForward->popupMenu()->clear();
  m_paForward->fillHistoryPopup( m_currentView->history(), 0L, false, true );
}

void KonqMainView::slotGoMenuAboutToShow()
{
  m_paBack->fillGoMenu( m_currentView->history() );
}

void KonqMainView::slotUpActivated( int id )
{
  QString url = m_paUp->popupMenu()->text( id );
  KURL::encode(url); // re-encode
  openURL( 0L, KURL( url ) );
}

void KonqMainView::slotGoHistoryActivated( int steps )
{
  m_currentView->go( steps );
}

void KonqMainView::slotBackActivated( int id )
{
  m_currentView->go( -(m_paBack->popupMenu()->indexOf( id ) + 1) );
}

void KonqMainView::slotForwardActivated( int id )
{
  m_currentView->go( m_paForward->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainView::slotComboPlugged()
{
  m_combo = m_paURLCombo->combo();

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Settings" );
  QStringList locationBarCombo = config->readListEntry( "ToolBarCombo" );

  while ( locationBarCombo.count() > 10 )
    locationBarCombo.remove( locationBarCombo.fromLast() );

  if ( locationBarCombo.count() == 0 )
    locationBarCombo << QString();

  m_combo->clear();
  m_combo->insertStringList( locationBarCombo );
  m_combo->setCurrentItem( 0 );
}

void KonqMainView::slotShowMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
}

/*
void KonqMainView::slotShowStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}
*/

void KonqMainView::slotShowToolBar()
{
  toggleBar( "mainToolBar", "KToolBar" );
}

void KonqMainView::slotShowLocationBar()
{
  toggleBar( "locationToolBar", "KToolBar" );
}

void KonqMainView::slotShowBookmarkBar()
{
  toggleBar( "bookmarkToolBar", "KToolBar" );
}

void KonqMainView::toggleBar( const char *name, const char *className )
{
  KToolBar *bar = (KToolBar *)child( name, className );
  if ( !bar )
    return;
  if ( bar->isVisible() )
    bar->hide();
  else
    bar->show();
}

void KonqMainView::slotFullScreenStart()
{
  KonqFrame *widget = m_currentView->frame();
  m_tempContainer = widget->parentContainer();
  m_tempFocusPolicy = widget->focusPolicy();
  widget->statusbar()->hide();
  widget->recreate( 0L, WStyle_Customize | WStyle_NoBorder | WType_Popup, QPoint( 0, 0 ), true );
  widget->resize( QApplication::desktop()->width(), QApplication::desktop()->height() );

  attachToolbars( widget );

  widget->setFocusPolicy( QWidget::StrongFocus );
  widget->setFocus();
  m_bFullScreen = true;
}

void KonqMainView::attachToolbars( KonqFrame *frame )
{
  QWidget *toolbar = guiFactory()->container( "locationToolBar", this );
  ((KToolBar*)toolbar)->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
    toolbar->recreate( frame, 0, QPoint( 0,0 ), true );
  frame->layout()->insertWidget( 0, toolbar );

  toolbar = guiFactory()->container( "mainToolBar", this );
  ((KToolBar*)toolbar)->setEnableContextMenu(false);
  if ( toolbar->parentWidget() != frame )
    toolbar->recreate( frame, 0, QPoint( 0, 0 ), true );
  frame->layout()->insertWidget( 0, toolbar );
}

void KonqMainView::slotFullScreenStop()
{
  QWidget *toolbar1 = guiFactory()->container( "mainToolBar", this );
  QWidget *toolbar2 = guiFactory()->container( "locationToolBar", this );

  ((KToolBar*)toolbar1)->setEnableContextMenu(true);
  ((KToolBar*)toolbar2)->setEnableContextMenu(true);

  KonqFrame *widget = m_currentView->frame();
  widget->close( false );
  widget->recreate( m_tempContainer, 0, QPoint( 0, 0 ), true);
  widget->statusbar()->show();
  widget->setFocusPolicy( m_tempFocusPolicy );
  m_bFullScreen = false;

  widget->attachInternal();

  toolbar1->recreate( this, 0, QPoint( 0, 0 ), true );
  toolbar2->recreate( this, 0, QPoint( 0, 0 ), true );
}

void KonqMainView::setLocationBarURL( KonqChildView *childView, const QString &url )
{
  childView->setLocationBarURL( url );

  if ( childView == (KonqChildView *)m_currentView && m_combo )
    m_combo->setEditText( url );

}

void KonqMainView::startAnimation()
{
  m_animatedLogoCounter = 0;
  m_animatedLogoTimer.start( 50 );
  m_paStop->setEnabled( true );
}

void KonqMainView::stopAnimation()
{
  m_animatedLogoTimer.stop();
  m_paAnimatedLogo->setIconSet( QIconSet( *( s_plstAnimatedLogo->at( 0 ) ) ) );

  m_paStop->setEnabled( false );
}

void KonqMainView::setUpEnabled( const KURL &url )
{
  bool bHasUpURL = false;

  if ( url.hasPath() )
    bHasUpURL = ( url.path() != "/" );
  if ( !bHasUpURL )
    bHasUpURL = url.hasSubURL();

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainView::initActions()
{
  // File menu
  /*
  KActionMenu * m_pamFile = (KActionMenu *)(actionCollection()->action("file_menu"));
  QObject::connect( m_pamFile->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );
                    */

  m_pMenuNew = new KNewMenu ( actionCollection(), "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paNewWindow = new KAction( i18n( "New &Window" ), QIconSet( BarIcon( "filenew",  KonqFactory::instance() ) ), KStdAccel::key(KStdAccel::New), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );

  QPixmap execpix = KGlobal::iconLoader()->loadIcon( "exec", KIconLoader::Small );
  m_paRun = new KAction( i18n( "&Run Command..." ), execpix, 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  QPixmap terminalpix = KGlobal::iconLoader()->loadIcon( "terminal", KIconLoader::Small );
  m_paOpenTerminal = new KAction( i18n( "Open &Terminal..." ), terminalpix, CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
	m_paOpenLocation = new KAction( i18n( "&Open Location..." ), QIconSet( BarIcon( "fileopen", KonqFactory::instance() ) ), KStdAccel::key(KStdAccel::Open), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );
  m_paToolFind = new KAction( i18n( "&Find" ), QIconSet( BarIcon( "find",  KonqFactory::instance() ) ), 0 /*not KStdAccel::find!*/, this, SLOT( slotToolFind() ), actionCollection(), "find" );

  m_paPrint = KStdAction::print( this, SLOT( slotPrint() ), actionCollection(), "print" );
  m_paShellClose = KStdAction::close( this, SLOT( close() ), actionCollection(), "close" );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use HTML" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
  m_ptaShowDirTree = new KToggleAction( i18n( "Show Directory Tree" ), 0, actionCollection(), "showdirtree" );
  connect( m_ptaShowDirTree, SIGNAL( toggled( bool ) ), this, SLOT( slotToggleDirTree( bool ) ) );

  // Go menu
  m_paUp = new KonqHistoryAction( i18n( "&Up" ), QIconSet( BarIcon( "up", KonqFactory::instance() ) ), CTRL+Key_Up, actionCollection(), "up" );

  connect( m_paUp, SIGNAL( activated() ), this, SLOT( slotUp() ) );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  m_paBack = new KonqHistoryAction( i18n( "&Back" ), QIconSet( BarIcon( "back", KonqFactory::instance() ) ), CTRL+Key_Left, actionCollection(), "back" );

  m_paBack->setEnabled( false );

  connect( m_paBack, SIGNAL( activated() ), this, SLOT( slotBack() ) );
  // go menu
  connect( m_paBack, SIGNAL( menuAboutToShow() ), this, SLOT( slotGoMenuAboutToShow() ) );
  connect( m_paBack, SIGNAL( activated( int ) ), this, SLOT( slotGoHistoryActivated( int ) ) );
  // toolbar button
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KonqHistoryAction( i18n( "&Forward" ), QIconSet( BarIcon( "forward", KonqFactory::instance() ) ), CTRL+Key_Right, actionCollection(), "forward" );

  m_paForward->setEnabled( false );

  connect( m_paForward, SIGNAL( activated() ), this, SLOT( slotForward() ) );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHome = KStdAction::home( this, SLOT( slotHome() ), actionCollection(), "home" );

  (void) new KAction( i18n( "File &Types" ), 0, this, SLOT( slotGoMimeTypes() ), actionCollection(), "go_mimetypes" );
  (void) new KAction( i18n( "App&lications" ), 0, this, SLOT( slotGoApplications() ), actionCollection(), "go_applications" );
  (void) new KAction( i18n( "Directory Tree" ), 0, this, SLOT( slotGoDirTree() ), actionCollection(), "go_dirtree" );
  (void) new KAction( i18n( "Trash" ), 0, this, SLOT( slotGoTrash() ), actionCollection(), "go_trash" );
  // Perhaps we should add Templates and Autostart as well ?

  // Options menu
  m_paSaveSettings = new KAction( i18n( "Sa&ve Settings" ), 0, this, SLOT( slotSaveSettings() ), actionCollection(), "savePropertiesAsDefault" );
  m_paSaveLocalProperties = new KAction( i18n( "Save Settings for this &URL" ), 0, actionCollection(), "saveLocalProperties" );

  m_paConfigureFileManager = new KAction( i18n( "Configure File &Manager..." ), 0, this, SLOT( slotConfigureFileManager() ), actionCollection(), "configurefilemanager" );
  m_paConfigureBrowser = new KAction( i18n( "Configure &Browser..." ), 0, this, SLOT( slotConfigureBrowser() ), actionCollection(), "configurebrowser" );
  m_paConfigureFileTypes = new KAction( i18n( "Configure File &Associations..." ), 0, this, SLOT( slotConfigureFileTypes() ), actionCollection(), "configurefiletypes" );
  m_paConfigureNetwork = new KAction( i18n( "Configure &Network..." ), 0, this, SLOT( slotConfigureNetwork() ), actionCollection(), "configurenetwork" );
  m_paConfigureKeys = new KAction( i18n( "Configure &Keys..." ), 0, this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );

  m_paConfigureToolbars = new KAction( i18n( "Configure Tool&bars..." ), 0, this, SLOT( slotConfigureToolbars() ), actionCollection(), "configuretoolbars" );

  m_paSplitViewHor = new KAction( i18n( "Split View &Horizontally" ), CTRL+SHIFT+Key_H, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Vertically" ), CTRL+SHIFT+Key_V, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "Split Window Horizontally" ), 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "Split Window Vertically" ), 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "Remove Active View" ), CTRL+SHIFT+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  m_paSaveDefaultProfile = new KAction( i18n( "Save Current Profile As Default" ), 0, this, SLOT( slotSaveDefaultProfile() ), actionCollection(), "savedefaultprofile" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "Save/Remove View Profile" ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_paFullScreenStart = new KAction( i18n( "Experimental Fullscreen Mode" ), 0, this, SLOT( slotFullScreenStart() ), actionCollection(), "fullscreenstart" );
  m_paFullScreenStop = new KAction( i18n( "Stop Experimental Fullscreen Mode" ), 0, this, SLOT( slotFullScreenStop() ), actionCollection(), "fullscreenstop" );

  /*
  QPixmap konqpix = KGlobal::iconLoader()->loadIcon( "konqueror", KIconLoader::Small );
  (void) new KAction( i18n( "&About Konqueror..." ), konqpix, 0, this, SLOT( slotAbout() ), actionCollection(), "about" );
  */

  KHelpMenu * m_helpMenu = new KHelpMenu( this );
  KStdAction::helpContents( m_helpMenu, SLOT( appHelpActivated() ), actionCollection(), "contents" );
  KStdAction::whatsThis( m_helpMenu, SLOT( contextHelpActivated() ), actionCollection(), "whats_this" );
  KStdAction::aboutApp( this, SLOT( slotAbout() ), actionCollection(), "about_app" );
  KStdAction::aboutKDE( m_helpMenu, SLOT( aboutKDE() ), actionCollection(), "about_kde" );
  KStdAction::reportBug( m_helpMenu, SLOT( reportBug() ), actionCollection(), "report_bug" );

  m_paReload = new KAction( i18n( "&Reload" ), QIconSet( BarIcon( "reload", KonqFactory::instance() ) ), KStdAccel::key(KStdAccel::Reload), this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paCut = KStdAction::cut( this, SLOT( slotCut() ), actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( this, SLOT( slotCopy() ), actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( this, SLOT( slotPaste() ), actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), QIconSet( BarIcon( "stop", KonqFactory::instance() ) ), Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  // Which is the default
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Trash" );
  int deleteAction = config->readNumEntry("DeleteAction", DEFAULT_DELETEACTION);
  const int deleteKey = CTRL+Key_Delete ; // Key_Delete conflict with the location bar

  m_paTrash = new KAction( i18n( "&Move to Trash" ), QIconSet( BarIcon( "trash", KonqFactory::instance() ) ), deleteAction==1 ? deleteKey : 0, this, SLOT( slotTrash() ), actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), deleteAction==2 ? deleteKey : 0, this, SLOT( slotDelete() ), actionCollection(), "del" );
  m_paShred = new KAction( i18n( "&Shred" ), deleteAction==3 ? deleteKey : 0, this, SLOT( slotShred() ), actionCollection(), "shred" );

  m_paAnimatedLogo = new KonqLogoAction( QString::null, QIconSet( *s_plstAnimatedLogo->at( 0 ) ), 0, this, SLOT( slotNewWindow() ), actionCollection(), "animated_logo" );

  (void)new KonqLabelAction( i18n( "Location " ), actionCollection(), "location_label" );

  m_paURLCombo = new KonqComboAction( i18n( "Location " ), 0, this, SLOT( slotURLEntered( const QString & ) ), actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( plugged() ),
           this, SLOT( slotComboPlugged() ) );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), actionCollection(), "bookmarks" );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_pamBookmarks->popupMenu(), actionCollection(), true );

  m_paShowMenuBar = KStdAction::showMenubar( this, SLOT( slotShowMenuBar() ), actionCollection(), "showmenubar" );
  //m_paShowStatusBar = KStdAction::showStatusbar( this, SLOT( slotShowStatusBar() ), actionCollection(), "showstatusbar" );
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( slotShowToolBar() ), actionCollection(), "showtoolbar" );
  m_paShowLocationBar = new KToggleAction( i18n( "Show &Locationbar" ), 0, actionCollection(), "showlocationbar" );
  m_paShowBookmarkBar = new KToggleAction( i18n( "Show &Bookmarkbar" ), 0, actionCollection(), "showbookmarkbar" );

  m_paShowLocationBar->setChecked( true );
  m_paShowBookmarkBar->setChecked( true );

  connect( m_paShowLocationBar, SIGNAL( activated() ), this, SLOT( slotShowLocationBar() ) );
  connect( m_paShowBookmarkBar, SIGNAL( activated() ), this, SLOT( slotShowBookmarkBar() ) );

  enableAllActions( false );
}

void KonqMainView::initPlugins()
{
  KTrader::OfferList offers = KTrader::self()->query( "Konqueror/Plugin" );
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();

  for (; it != end; ++it )
  {
    if ( (*it)->library().isEmpty() )
      continue;

    KLibFactory *factory = KLibLoader::self()->factory( (*it)->library() );
						
    if ( !factory )
      continue;

    (void)factory->create( this );
  }
}

void KonqMainView::updateStatusBar()
{
// now view specific
/*
  if ( !m_progressBar )
    return;

  int progress = m_currentView->progress();

  if ( progress != -1 )
  {
    if ( !m_progressBar->isVisible() )
      m_progressBar->show();
  }
  else
    m_progressBar->hide();

  m_progressBar->setValue( progress );

  assert(m_statusBar);
  m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  m_statusBar->changeItem( 0L, STATUSBAR_MSG_ID );
*/
}

void KonqMainView::updateToolBarActions()
{
  // Enables/disables actions that depend on the current view (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  if ( m_currentView->isLoading() )
    startAnimation(); // takes care of m_paStop
  else
    stopAnimation(); // takes care of m_paStop
}

QString KonqMainView::findIndexFile( const QString &dir )
{
  QDir d( dir );

  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( ".kde.html", false );
  if ( QFile::exists( f ) )
    return f;

  return QString::null;
}

void KonqMainView::connectExtension( KParts::BrowserExtension *ext )
{
  kDebugInfo( 1202, "connectExtension" );
  // "cut", "copy", "pastecut", "pastecopy", "del", "trash", "shred"
  // "reparseConfiguration", "refreshMimeTypes"
  // are not directly connected. The slots in this class are connected instead and
  // call in turn the ones in the view...

  // The ones on the first line have an intermediate slot because we do some konqy stuff
  // before calling the view

  // And the ones on the second line don't even have a corresponding action - it's just
  // a method that the view can have or not have.

  static const char * s_actionnames[] = {
    "cut", "copy", "paste", "del", "trash", "shred",
      "print", "saveLocalProperties", "savePropertiesAsDefault" };
  QStrList slotNames =  ext->metaObject()->slotNames();
  // Loop over standard action names
  for ( unsigned int i = 0 ; i < sizeof(s_actionnames)/sizeof(char*) ; i++ )
  {
    kDebugInfo( 1202, s_actionnames[i] );
    QAction * act = actionCollection()->action( s_actionnames[i] );
    assert(act);
    QCString slotName = QCString(s_actionnames[i])+"()";
    bool enable = false;
    // Does the extension have a slot with the name of this action ?
    if ( slotNames.contains( slotName ) )
    {
      // Connect ? (see comment about most actions)
      if ( !strcmp( s_actionnames[i], "print" )
        || !strcmp( s_actionnames[i], "saveLocalProperties" )
        || !strcmp( s_actionnames[i], "savePropertiesAsDefault" ) )
      {
        // OUCH. Here we use the fact that qobjectdefs.h:#define SLOT(a) "1"#a
        ext->connect( act, SIGNAL( activated() ), ext, QCString("1") + slotName );
        kDebugInfo( 1202, "Connecting to %s", s_actionnames[i] );
        enable = true;
      }
    }
    act->setEnabled( enable );
  }
  connect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainView::disconnectExtension( KParts::BrowserExtension *ext )
{
  kDebugInfo( 1202, "Disconnecting extension" );
  QValueList<QAction *> actions = actionCollection()->actions();
  QValueList<QAction *>::ConstIterator it = actions.begin();
  QValueList<QAction *>::ConstIterator end = actions.end();
  for (; it != end; ++it )
    (*it)->disconnect( ext );
  disconnect( ext, SIGNAL( enableAction( const char *, bool ) ),
           this, SLOT( slotEnableAction( const char *, bool ) ) );
}

void KonqMainView::slotEnableAction( const char * name, bool enabled )
{
  // Hmm, we have one action for paste, not two...
  QCString hackName( name );
  if ( hackName == "pastecopy" || hackName == "pastecut" )
    hackName = "paste";

  QAction * act = actionCollection()->action( hackName.data() );
  if (!act)
    kDebugWarning( 1202, "Unknown action %s - can't enable", hackName.data() );
  else
    act->setEnabled( enabled );
}

void KonqMainView::enableAllActions( bool enable )
{
  int count = actionCollection()->count();
  for ( int i = 0; i < count; i++ )
    actionCollection()->action( i )->setEnabled( enable );

  actionCollection()->action( "close" )->setEnabled( true );
}

void KonqMainView::openBookmarkURL( const QString & url )
{
  kDebugInfo(1202, "%s", QString("KonqMainView::openBookmarkURL(%1)").arg(url).latin1() );
  openURL( 0L, KURL( url ) );
}

void KonqMainView::setCaption( const QString &caption )
{
  // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
  m_title = caption;
  KParts::MainWindow::setCaption( caption );
}

QString KonqMainView::currentURL()
{
  assert( m_currentView );
  return m_currentView->view()->url().url();
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  KonqFileItem item( url, _mimeType, _mode );
  KonqFileItemList items;
  items.append( &item );
  slotPopupMenu( _global, items ); //BE CAREFUL WITH sender() !
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KonqFileItemList &_items )
{
  m_oldView = m_currentView;

  m_currentView = childView( (KParts::ReadOnlyPart *)sender()->parent() );

  //kDebugInfo( 1202, "KonqMainView::slotPopupMenu(...)");

  QActionCollection popupMenuCollection;
  if ( !menuBar()->isVisible() )
    popupMenuCollection.insert( m_paShowMenuBar );
  popupMenuCollection.insert( m_paBack );
  popupMenuCollection.insert( m_paForward );
  popupMenuCollection.insert( m_paUp );

  popupMenuCollection.insert( m_paCut );
  popupMenuCollection.insert( m_paCopy );
  popupMenuCollection.insert( m_paPaste );
  popupMenuCollection.insert( m_paTrash );
  popupMenuCollection.insert( m_paDelete );
  popupMenuCollection.insert( m_paShred );

  if ( m_bFullScreen )
    popupMenuCollection.insert( m_paFullScreenStop );

  KonqPopupMenu pPopupMenu ( _items,
                             m_currentView->url(),
                             popupMenuCollection,
                             m_pMenuNew );
  pPopupMenu.exec( _global );

  m_currentView = m_oldView;
}

void KonqMainView::slotDatabaseChanged()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    callExtensionMethod( (*it), "refreshMimeTypes()" );
}

void KonqMainView::reparseConfiguration()
{
  kDebugInfo( 1202, "KonqMainView::reparseConfiguration() !");
  kapp->config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();
  KonqHTMLSettings::reparseConfiguration();
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    callExtensionMethod( (*it), "reparseConfiguration()" );
}

static const char *viewModeGUI = ""
"<!DOCTYPE viewmodexml>"
"<viewmodexml name=\"viewmode\">"
"<MenuBar>"
" <Menu name=\"view\">"
"  <Menu name=\"viewmodes\">"
"  </Menu>"
" </Menu>"
"</MenuBar>"
"</viewmodexml>";

ViewModeGUIClient::ViewModeGUIClient( KonqMainView *mainView )
 : QObject( mainView )
{
  m_mainView = mainView;
  m_doc.setContent( QString::fromLatin1( viewModeGUI ) );
  m_menuElement = m_doc.documentElement().namedItem( "MenuBar" ).namedItem( "Menu" ).namedItem( "Menu" ).toElement();
  m_actions = 0L;
}

QAction *ViewModeGUIClient::action( const QDomElement &element )
{
  if ( !m_actions )
    return 0L;

  return m_actions->action( element.attribute( "name" ) );
}

QDomDocument ViewModeGUIClient::document() const
{
  return m_doc;
}

void ViewModeGUIClient::update( const KTrader::OfferList &services )
{
  if ( m_actions )
    delete m_actions;

  m_actions = new QActionCollection( this );

  QDomNode n = m_menuElement.firstChild();
  while ( !n.isNull() )
  {
    m_menuElement.removeChild( n );
    n = m_menuElement.firstChild();
  }

  if ( services.count() <= 1 )
    return;

  QDomElement textElement = m_doc.createElement( "text" );
  textElement.appendChild( m_doc.createTextNode( i18n( "View Mode..." ) ) );
  m_menuElement.appendChild( textElement );

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    KRadioAction *action = new KRadioAction( (*it)->comment(), 0, m_actions, (*it)->name() );

    QDomElement e = m_doc.createElement( "Action" );
    m_menuElement.appendChild( e );
    e.setAttribute( "name", (*it)->name() );

    if ( (*it)->name() == m_mainView->currentChildView()->service()->name() )
      action->setChecked( true );

    action->setExclusiveGroup( "KonqMainView_ViewModes" );

    connect( action, SIGNAL( toggled( bool ) ),
	     m_mainView, SLOT( slotViewModeToggle( bool ) ) );
  }
}

#include "konq_mainview.moc"
