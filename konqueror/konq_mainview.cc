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

#include <kbrowser.h>
#include "konq_htmlsettings.h"
#include "konq_mainview.h"
#include "konq_shell.h"
#include "konq_part.h"
#include "konq_childview.h"
#include "konq_run.h"
#include "konq_factory.h"
#include "konq_viewmgr.h"
#include "konq_frame.h"
#include "konq_events.h"
#include "konq_actions.h"

#include <pwd.h>
#include <netdb.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <qaction.h>
#include <qapplication.h>
#include <qclipboard.h>

#include <kaction.h>
#include <kdebug.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kio_cache.h>
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
#include <kio_job.h>
#include <kuserpaths.h>
#include <ktrader.h>
#include <ksharedptr.h>
#include <kservice.h>
#include <kapp.h>
#include <dcopclient.h>
#include <klibloader.h>
#include <part.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
#include <kstdaction.h>

#include <kbugreport.h>
#include "version.h"

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

template class QList<QPixmap>;
template class QList<KToggleAction>;

QList<QPixmap> *KonqMainView::s_plstAnimatedLogo = 0L;
bool KonqMainView::s_bMoveSelection = false;

KonqMainView::KonqMainView( KonqPart *part, QWidget *parent, const char *name )
 : View( part, parent, name ),  DCOPObject( "KonqMainViewIface" )
{
  setFocusPolicy( NoFocus ); // don't give the focus to this dummy QWidget

  m_currentView = 0L;
  m_pBookmarkMenu = 0L;
  m_bViewModeLock = false;
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

  m_viewModeActions.setAutoDelete( true );

  initActions();
  initPlugins();

  m_bMenuEditDirty = true;
  m_bMenuViewDirty = true;

  connect( QApplication::clipboard(), SIGNAL( dataChanged() ),
           this, SLOT( checkEditExtension() ) );
  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

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

void KonqMainView::openFilteredURL( KonqChildView */*_view*/, const QString &_url )
{
  KonqURLEnterEvent ev( konqFilteredURL( _url ) );
  QApplication::sendEvent( this, &ev );
}

void KonqMainView::openURL( KonqChildView *_view, const QString &_url, bool reload, int xOffset,
                            int yOffset, const QString &serviceType )
{
  debug("%s", QString("KonqMainView::openURL : _url = '%1'").arg(_url).latin1());

  /////////// First, modify the URL if necessary (adding protocol, ...) //////

  QString url = konqFilteredURL(_url);

  KURL u( url );
  if ( u.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(url);
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
    {
      view->stop();
    }	

    //kdebug( KDEBUG_INFO, 1202, "%s", QString("view->run for %1").arg(url).latin1() );
     setLocationBarURL( view, url );
     view->setMiscURLData( reload, xOffset, yOffset );
    if ( !serviceType.isEmpty() )
    {
      if ( !openView( serviceType, url, view ) )
        (void)new KRun( url );
    }
    else
      view->run( url );

  }
  else
  {
    //kdebug( KDEBUG_INFO, 1202, "%s", QString("Creating new konqrun for %1").arg(url).latin1() );
    if ( m_combo )
      m_combo->setEditText( url );

    if ( !serviceType.isEmpty() )
    {
      if ( !openView( serviceType, url, 0L ) )
        (void)new KRun( url );
    }
    else
      (void) new KonqRun( this, 0L, url, 0, false, true );
  }

  if ( view && view == m_currentView && serviceType.isEmpty() )
  {
    view->setLoading( true );
    startAnimation();
  }
}

void KonqMainView::openURL( const QString &url, bool reload, int xOffset,
                            int yOffset, const QString &serviceType )
{
  KonqChildView *childV = 0L;

  if ( sender() )
    childV = childView( (BrowserView *)sender() );

  openURL( childV, url, reload, xOffset, yOffset, serviceType );
}

void KonqMainView::slotCreateNewWindow( const QString &url )
{
 KonqFileManager::getFileManager()->openFileManagerWindow( url );
}

void KonqMainView::slotNewWindow()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( m_currentView->view()->url() );
}

void KonqMainView::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. KWM does not support that yet
  KWM::sendKWMCommand("execute");
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
  system( cmd.data() );
}

void KonqMainView::slotOpenLocation()
{
  QString u;

  if (m_currentView)
    u = m_currentView->url();

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

  KURL url;
  if ( m_currentView )
    url = m_currentView->url();

  if( url.isLocalFile() )
    proc << url.directory();

  proc.start(KShellProcess::DontCare);
}

void KonqMainView::slotPrint()
{
  QObject *obj = m_currentView->view()->child( 0L, "PrintingExtension" );
  if ( obj )
    ((PrintingExtension *)obj)->print();
}

void KonqMainView::slotViewModeToggle( bool toggle )
{
  if ( !toggle )
    return;

  QString modeName = sender()->name();

  if ( m_currentView->service()->name() == modeName )
    return;

  m_bViewModeLock = true;

  m_currentView->lockHistory();
  m_currentView->changeViewMode( m_currentView->serviceType(), QString::null,
                                 false, modeName );
				
  m_bViewModeLock = false;
}

void KonqMainView::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

  m_currentView->setAllowHTML( b );

  m_bMenuViewDirty = true;

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

    BrowserView *view = m_pViewManager->split( splitFrame, Qt::Horizontal, QString::fromLatin1( "Browser/View" ), QString::fromLatin1( "KonqDirTree" ), &newContainer );

    newContainer->moveToLast( splitFrame->widget() );

    QValueList<int> newSplitterSizes;
    newSplitterSizes << 30 << 100;

    newContainer->setSizes( newSplitterSizes );

    KonqChildView *cv = childView( view );

    cv->setPassiveMode( true );
    cv->frame()->header()->passiveModeCheckBox()->setChecked( true );
  }
  else
  {
    QList<KonqChildView> viewList;

    mainContainer->listViews( &viewList );

    QListIterator<KonqChildView> it( viewList );

    for (; it.current(); ++it )
      if ( it.current()->view()->inherits( "KonqDirTreeBrowserView" ) )
        m_pViewManager->removeView( it.current() );
  }
}

void KonqMainView::slotStop()
{
  if ( m_currentView )
  {
    m_currentView->stop();
    stopAnimation();

    if ( m_progressBar )
    {
      m_progressBar->setValue( -1 );
      m_progressBar->hide();
    }

    if ( m_statusBar )
    {
      m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
      m_statusBar->changeItem( 0L, STATUSBAR_MSG_ID );
    }

  }
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotUp()
{
  QString url = m_currentView->url();
  KURL u( url );

  openURL( (KonqChildView *)m_currentView, u.upURL(true).url() );
}

void KonqMainView::slotBack()
{
  m_currentView->goBack();

  m_paBack->setEnabled( m_currentView->canGoBack() );
}

void KonqMainView::slotForward()
{
  m_currentView->goForward();

  m_paBack->setEnabled( m_currentView->canGoForward() );
}

void KonqMainView::slotHome()
{
  openURL( 0L, KonqFMSettings::defaultIconSettings()->homeURL() );
}

void KonqMainView::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    KMessageBox::sorry( 0L, i18n( "Could not write index file" ));
    return;
  }

  QString f = file;
  KURL::encode( f );
  f.prepend( "file:" );
  openURL( (KonqChildView *)m_currentView, f );
}

void KonqMainView::slotShowHistory()
{
  // TODO
}

void KonqMainView::slotEditMimeTypes()
{
  openURL( (KonqChildView *)m_currentView, KonqFactory::instance()->dirs()->saveLocation("mime").prepend( "file:" ) );
}

void KonqMainView::slotEditApplications()
{
  openURL( (KonqChildView *)m_currentView, KonqFactory::instance()->dirs()->saveLocation("apps").prepend( "file:" ) );
}

void KonqMainView::slotEditDirTree()
{
  KonqFileManager::getFileManager()->openFileManagerWindow( locateLocal( "data", "konqueror/dirtree/" ) );
}

void KonqMainView::slotSaveSettings()
{
  QObject *obj = m_currentView->view()->child( 0L, "ViewPropertiesExtension" );
  if ( obj )
    ((ViewPropertiesExtension *)obj)->savePropertiesAsDefault();
}

void KonqMainView::slotSaveSettingsPerURL()
{
  QObject *obj = m_currentView->view()->child( 0L, "ViewPropertiesExtension" );
  if ( obj )
    ((ViewPropertiesExtension *)obj)->saveLocalProperties();
}

void KonqMainView::slotConfigureFileManager()
{
  if (fork() == 0) {
    execl(locate("exe", "kcmshell"), "kcmshell", "Applications/konqueror", 0);
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
  QActionDialog *actionDia = new QActionDialog( actionCollection(), this, 0 );
  actionDia->show();
}

void KonqMainView::slotViewChanged( BrowserView *oldView, BrowserView *newView )
{
  m_mapViews.remove( oldView );
  m_mapViews.insert( newView, (KonqChildView *)sender() );

  if ( (KonqChildView *)sender() == (KonqChildView *)m_currentView )
  {
    unPlugViewGUI( oldView );
    plugInViewGUI( newView );
    updateStatusBar();
    checkEditExtension();
    m_bMenuViewDirty = true;
  }
}

void KonqMainView::slotStarted()
{
  BrowserView *view = (BrowserView *)sender();

  QString url = view->url();

  qDebug( "void KonqMainView::slotStarted() %s", url.ascii() );

  MapViews::ConstIterator it = m_mapViews.find( view );

  assert( it != m_mapViews.end() );

  (*it)->setLoading( true );
  (*it)->setViewStarted( true );

  (*it)->makeHistory( true );

  if ( (KonqChildView *)m_currentView == *it )
  {
    updateStatusBar();
    updateToolBarActions();
  }

}

void KonqMainView::slotCompleted()
{
  BrowserView *view = (BrowserView *)sender();

  MapViews::ConstIterator it = m_mapViews.find( view );

  (*it)->setLoading( false );
  (*it)->setViewStarted( false );
  (*it)->setProgress( -1 );

  if ( (KonqChildView *)m_currentView == *it )
  {
    updateToolBarActions();

    if ( m_progressBar )
    {
      m_progressBar->setValue( -1 );
      m_progressBar->hide();
    }

    if ( m_statusBar )
      m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  }

}

void KonqMainView::slotCanceled()
{
  slotCompleted();
}

void KonqMainView::slotRunFinished()
{
  KonqRun *run = (KonqRun *)sender();

  if ( run->foundMimeType() )
    return;

  KonqChildView *childView = run->childView();

  if ( !childView )
    return;

  childView->setLoading( false );

  if ( childView == m_currentView )
  {
    stopAnimation();
    setLocationBarURL( childView, childView->view()->url() );
  }
}

void KonqMainView::slotSetStatusBarText( const QString &text )
{
  if ( !m_statusBar )
    return;

  m_statusBar->changeItem( text, STATUSBAR_MSG_ID );
}

bool KonqMainView::openView( QString serviceType, QString url, KonqChildView *childView )
{
debug(" KonqMainView::openView %s %s", serviceType.ascii(), url.ascii());
  QString indexFile;

  if ( !childView )
    {
      BrowserView* view = m_pViewManager->splitView( Qt::Horizontal, url, serviceType );

      if ( !view )
      {
        KMessageBox::sorry( 0L, i18n( "Could not create view for %1\nCheck your installation").arg(serviceType) );
        return true; // fake everything was ok, we don't want to propagate the error
      }

      setActiveView( view );

      enableAllActions( true ); // can't we rely on setActiveView to do the right thing ? (David)
      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );

      view->setFocus();

      return true;
    }
  else
  {
    debug("%s", QString("(1) KonqMainView::openView : url = '%1'").arg(url).latin1());
    // This is already called in ::openURL
    //    childView->stop();
  }
  debug("%s", QString("(2) KonqMainView::openView : url = '%1'").arg(url).latin1());

  KURL u( url );
  if ( ( serviceType == "inode/directory" ) &&
       ( childView->allowHTML() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
  {
    serviceType = "text/html";
    url = indexFile.prepend( "file:" );
  }

  if ( childView->changeViewMode( serviceType, url ) )
  {
    // Give focus to the view
    childView->view()->setFocus();
    return true;
  }

  // Didn't work, abort
  childView->setLoading( false );
  if ( childView == m_currentView )
  {
    stopAnimation();
    setLocationBarURL( childView, childView->view()->url() );
  }

  return false;
}

void KonqMainView::setActiveView( BrowserView *view )
{
  KonqChildView *newView = m_mapViews.find( view ).data();

  if ( newView->passiveMode() )
    return;

  KonqChildView *oldView = m_currentView;

  if ( m_currentView )
    unPlugViewGUI( m_currentView->view() );

  m_currentView = newView;

  m_currentView->frame()->header()->repaint();

  if ( oldView )
    oldView->frame()->header()->repaint();

  plugInViewGUI( view );

  if ( m_combo )
    m_combo->setEditText( m_currentView->locationBarURL() );

  updateStatusBar();
  updateToolBarActions();

  m_bMenuViewDirty = true;
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
    unPlugViewGUI( m_currentView->view() );
    m_currentView = 0L;
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

KonqChildView *KonqMainView::childView( BrowserView *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

QValueList<BrowserView *> KonqMainView::viewList()
{
  QValueList<BrowserView *> res;

  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    res.append( (*it)->view() );

  return res;
}

BrowserView *KonqMainView::currentView()
{
  if ( m_currentView )
    return m_currentView->view();
  else
    return 0L;
}

void KonqMainView::customEvent( QCustomEvent *event )
{
  View::customEvent( event );

  if ( KonqURLEnterEvent::test( event ) )
  {
    QString url = ((KonqURLEnterEvent *)event)->url();

    openURL( 0L, url );

    return;
  }
}

void KonqMainView::resizeEvent( QResizeEvent * )
{
  m_pViewManager->doGeometry( width(), height() );
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
  m_pMenuNew->setPopupFiles( m_currentView->url() );
}

void KonqMainView::slotMenuEditAboutToShow()
{
  if ( !m_bMenuEditDirty )
    return;

  m_bMenuEditDirty = false;
}

void KonqMainView::slotMenuViewAboutToShow()
{
  if ( !m_bMenuViewDirty || !m_pamView->isEnabled() )
    return;

  qDebug( "void KonqMainView::slotMenuViewAboutToShow()" );

  if ( !m_bViewModeLock )
  {
    qDebug( "no lock!" );
    m_pamView->popupMenu()->clear(); //remove separators
    m_pamView->remove( m_ptaUseHTML );
    m_pamView->remove( m_ptaShowDirTree );
    m_pamView->remove( m_paReload );

    m_viewModeActions.clear();

    KTrader::OfferList services = m_currentView->serviceOffers();

    if ( services.count() > 1 )
    {
      KTrader::OfferList::ConstIterator it = services.begin();
      KTrader::OfferList::ConstIterator end = services.end();

      for (; it != end; ++it )
      {
        KToggleAction *action = new KToggleAction( (*it)->comment(), 0, this, (*it)->name() );

        m_viewModeActions.append( action );
        m_pamView->insert( action );

        if ( (*it)->name() == m_currentView->service()->name() )
          action->setChecked( true );
	
        action->setExclusiveGroup( "KonqMainView_ViewModes" );

        connect( action, SIGNAL( toggled( bool ) ),
                 this, SLOT( slotViewModeToggle( bool ) ) );
      }

      m_pamView->popupMenu()->insertSeparator();
    }

    m_pamView->insert( m_ptaUseHTML );
    m_pamView->insert( m_ptaShowDirTree );
    m_pamView->popupMenu()->insertSeparator();
    m_pamView->insert( m_paReload );

    // hhhhhmmmmmmmmmmmmmm... (Simon)
    m_ptaUseHTML->blockSignals( true );
    m_ptaUseHTML->setChecked( m_currentView->allowHTML() );
    m_ptaUseHTML->blockSignals( false );

    m_pamView->popupMenu()->insertSeparator();

  }

  const QValueList<BrowserView::ViewAction> *actions = m_currentView->view()->actions();
  QValueList<BrowserView::ViewAction>::ConstIterator it = actions->begin();
  QValueList<BrowserView::ViewAction>::ConstIterator end = actions->end();
  for (; it != end; ++it )
    if ( ( (*it).m_flags & BrowserView::MenuView ) == BrowserView::MenuView )
     (*it).m_action->plug( m_pamView->popupMenu() );

  m_bMenuViewDirty = false;
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
	
  setActiveView( nextView->view() );

  m_pViewManager->removeView( prevView );

  //if ( m_mapViews.count() == 1 )
  if ( m_pViewManager->chooseNextView((KonqChildView *)m_currentView) == 0L )
    m_currentView->frame()->header()->passiveModeCheckBox()->hide();
}

void KonqMainView::slotSaveDefaultProfile()
{
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Default View Profile" );
  m_pViewManager->saveViewProfile( *config );
}

void KonqMainView::slotLoadingProgress( int percent )
{
  if ( !m_progressBar )
    return;

  BrowserView *view = (BrowserView *)sender();

  if ( !view )
    return;

  if ( m_currentView->view() == view && m_currentView->isLoading() )
  {
    if ( !m_progressBar->isVisible() )
      m_progressBar->show();

    m_progressBar->setValue( percent );
  }

  KonqChildView *child = childView( view );
  if ( child )
    child->setProgress( percent );
}

void KonqMainView::slotSpeedProgress( int bytesPerSecond )
{
  BrowserView *view = (BrowserView *)sender();

  if ( !m_statusBar || m_currentView->view() != view || !m_currentView->isLoading() )
    return;

  QString sizeStr;

  if ( bytesPerSecond > 0 )
    sizeStr = KIOJob::convertSize( bytesPerSecond ) + QString::fromLatin1( "/s" );
  else
    sizeStr = i18n( "stalled" );

  m_statusBar->changeItem( sizeStr, STATUSBAR_SPEED_ID );
}

void KonqMainView::checkEditExtension()
{
  bool bCut = false;
  bool bCopy = false;
  bool bPaste = false;
  bool bMove = false;

  if ( m_currentView && m_currentView->view() )
  {
    QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
    if ( obj )
      ((EditExtension *)obj)->can( bCut, bCopy, bPaste, bMove );
  }

  m_paCut->setEnabled( bCut );
  m_paCopy->setEnabled( bCopy );
  m_paPaste->setEnabled( bPaste );
  m_paTrash->setEnabled( bMove );
  m_paDelete->setEnabled( bMove ); // should we do this for the trash can?
}

void KonqMainView::slotCut()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( !obj )
    return;

  ((EditExtension *)obj)->cutSelection();

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)true;
  kapp->dcopClient()->send( "*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = true;
}

void KonqMainView::slotCopy()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->copySelection();

  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << (int)false;
  kapp->dcopClient()->send( "*", "KonquerorIface", "setMoveSelection(int)", data );
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "setMoveSelection(int)", data );
  s_bMoveSelection = false;
}

void KonqMainView::slotPaste()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->pasteSelection( s_bMoveSelection );
}

void KonqMainView::slotTrash()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->moveSelection( KUserPaths::trashPath() );
}

void KonqMainView::slotDelete()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );

  if ( !obj )
    return;

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Misc Defaults" );
  bool confirm = config->readBoolEntry( "ConfirmDestructive", true );
  if (confirm)
  {
    QStringList selectedUrls = ((EditExtension *)obj)->selectedUrls();

    QStringList::Iterator it = selectedUrls.begin();
    QStringList::Iterator end = selectedUrls.end();
    for ( ; it != end; ++it )
      KURL::decode( *it );

    if ( KMessageBox::questionYesNoList(0, i18n( "Do you really want to delete the file(s) ?" ),
         selectedUrls )
	 == KMessageBox::No)
      return;
  }

  ((EditExtension *)obj)->moveSelection();
}

void KonqMainView::slotSetLocationBarURL( const QString &url )
{
  BrowserView *view = (BrowserView *)sender();
  setLocationBarURL( childView( view ), url );
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

void KonqMainView::slotReportBug()
{
  KBugReport dlg;
  dlg.exec();
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
  fillHistoryPopup( m_paBack->popupMenu(), m_currentView->backHistory() );
}

void KonqMainView::slotForwardAboutToShow()
{
  fillHistoryPopup( m_paForward->popupMenu(), m_currentView->forwardHistory() );
}

void KonqMainView::slotUpActivated( int id )
{
  openURL( 0L, m_paUp->popupMenu()->text( id ) );
}

void KonqMainView::slotBackActivated( int id )
{
  m_currentView->goBack( m_paBack->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainView::slotForwardActivated( int id )
{
  m_currentView->goForward( m_paForward->popupMenu()->indexOf( id ) + 1 );
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

void KonqMainView::fillHistoryPopup( QPopupMenu *menu, const QList<HistoryEntry> &history )
{
  menu->clear();

  QListIterator<HistoryEntry> it( history );
  uint i = 0;
  for (; it.current(); ++it )
  {
    menu->insertItem( KMimeType::mimeType( it.current()->strServiceType )->pixmap( KIconLoader::Small ),
                      it.current()->strURL );
    if ( ++i > 10 )
      break;
  }

}

void KonqMainView::setLocationBarURL( KonqChildView *childView, const QString &url )
{
  childView->setLocationBarURL( url );

  if ( childView == (KonqChildView *)m_currentView && m_combo )
    m_combo->setEditText( url );

}

void KonqMainView::viewActivateEvent( ViewActivateEvent *e )
{
  if ( e->deactivated() )
    return;

  m_statusBar = shell()->createStatusBar();

  m_progressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, m_statusBar );

  m_statusBar->insertWidget( m_progressBar, 120, STATUSBAR_LOAD_ID );
  m_statusBar->insertItem( QString::fromLatin1( "XXXXXXXX" ), STATUSBAR_SPEED_ID );
  m_statusBar->insertItem( 0L, STATUSBAR_MSG_ID );

  m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );

  m_statusBar->show();

  m_progressBar->hide();
  if ( m_currentView )
    setLocationBarURL( m_currentView, m_currentView->locationBarURL() );

  View::viewActivateEvent( e );
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

void KonqMainView::setUpEnabled( const QString &url )
{
  bool bHasUpURL = false;

  KURL u( url );
  if ( u.hasPath() )
    bHasUpURL = ( u.path() != "/" );
  if ( !bHasUpURL )
    bHasUpURL = u.hasSubURL();

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainView::initActions()
{
  KStdAccel stdAccel;

  // File menu
  /*
    TODO in the next kparts
  KActionMenu * m_pamFile = (KActionMenu *)(actionCollection()->action("file_menu"));
  QObject::connect( m_pamFile->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );
                    */

  m_pMenuNew = new KNewMenu ( actionCollection(), "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paNewWindow = new KAction( i18n( "New &Window" ), QIconSet( BarIcon( "filenew",  KonqFactory::instance() ) ), stdAccel.openNew(), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );

  QPixmap execpix = KGlobal::iconLoader()->loadIcon( "exec", KIconLoader::Small );
  m_paRun = new KAction( i18n( "&Run..." ), execpix, 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  QPixmap terminalpix = KGlobal::iconLoader()->loadIcon( "terminal", KIconLoader::Small );
  m_paOpenTerminal = new KAction( i18n( "Open &Terminal..." ), terminalpix, CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  m_paOpenLocation = new KAction( i18n( "&Open Location..." ), QIconSet( BarIcon( "fileopen", KonqFactory::instance() ) ), stdAccel.open(), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );
  m_paToolFind = new KAction( i18n( "&Find" ), QIconSet( BarIcon( "find",  KonqFactory::instance() ) ), 0 /*not stdAccel.find()!*/, this, SLOT( slotToolFind() ), actionCollection(), "find" );

  m_paPrint = KStdAction::print( this, SLOT( slotPrint() ), actionCollection(), "print" );

  // Edit menu
  m_pamEdit = new KActionMenu( i18n( "&Edit" ), actionCollection(), "edit_menu" );

  connect( m_pamEdit->popupMenu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotMenuEditAboutToShow() ) );

  // View menu
  m_pamView = new KActionMenu( i18n( "&View" ), actionCollection(), "view_menu" );

  QPopupMenu *popup = m_pamView->popupMenu();

  connect( popup, SIGNAL( aboutToShow() ),
           this, SLOT( slotMenuViewAboutToShow() ) );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use HTML" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
  m_ptaShowDirTree = new KToggleAction( i18n( "Show Directory Tree" ), 0, actionCollection(), "showdirtree" );
  connect( m_ptaShowDirTree, SIGNAL( toggled( bool ) ), this, SLOT( slotToggleDirTree( bool ) ) );

  // Go menu : TODO : connect to abouttoshow and append max 10 history items
  m_paUp = new KonqHistoryAction( i18n( "&Up" ), QIconSet( BarIcon( "up", KonqFactory::instance() ) ), ALT+Key_Up, actionCollection(), "up" );

  connect( m_paUp, SIGNAL( activated() ), this, SLOT( slotUp() ) );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  m_paBack = new KonqHistoryAction( i18n( "&Back" ), QIconSet( BarIcon( "back", KonqFactory::instance() ) ), ALT+Key_Left, actionCollection(), "back" );

  m_paBack->setEnabled( false );

  connect( m_paBack, SIGNAL( activated() ), this, SLOT( slotBack() ) );
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KonqHistoryAction( i18n( "&Forward" ), QIconSet( BarIcon( "forward", KonqFactory::instance() ) ), ALT+Key_Right, actionCollection(), "forward" );

  m_paForward->setEnabled( false );

  connect( m_paForward, SIGNAL( activated() ), this, SLOT( slotForward() ) );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHome = KStdAction::home( this, SLOT( slotHome() ), actionCollection(), "home" );

  m_paCache = new KAction( i18n( "&Cache" ), 0, this, SLOT( slotShowCache() ), actionCollection(), "cache" );
  //m_paHistory = new KAction( i18n( "&History" ), 0, this, SLOT( slotShowHistory() ), actionCollection(), "history" );
  m_paMimeTypes = new KAction( i18n( "File &Types" ), 0, this, SLOT( slotEditMimeTypes() ), actionCollection(), "mimetypes" );
  m_paApplications = new KAction( i18n( "App&lications" ), 0, this, SLOT( slotEditApplications() ), actionCollection(), "applications" );
  m_paDirTree = new KAction( i18n( "Directory Tree" ), 0, this, SLOT( slotEditDirTree() ), actionCollection(), "dirtree" );

  // Options menu
  m_paSaveSettings = new KAction( i18n( "Sa&ve Settings" ), 0, this, SLOT( slotSaveSettings() ), actionCollection(), "savesettings" );
  m_paSaveSettingsPerURL = new KAction( i18n( "Save Settings for this &URL" ), 0, this, SLOT( slotSaveSettingsPerURL() ), actionCollection(), "savesettingsperurl" );

  m_paConfigureFileManager = new KAction( i18n( "Configure File &Manager..." ), 0, this, SLOT( slotConfigureFileManager() ), actionCollection(), "configurefilemanager" );
  m_paConfigureBrowser = new KAction( i18n( "Configure &Browser..." ), 0, this, SLOT( slotConfigureBrowser() ), actionCollection(), "configurebrowser" );
  m_paConfigureFileTypes = new KAction( i18n( "Configure File &Associations..." ), 0, this, SLOT( slotConfigureFileTypes() ), actionCollection(), "configurefiletypes" );
  m_paConfigureNetwork = new KAction( i18n( "Configure &Network..." ), 0, this, SLOT( slotConfigureNetwork() ), actionCollection(), "configurenetwork" );
  m_paConfigureKeys = new KAction( i18n( "Configure &Keys..." ), 0, this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );

  m_paSplitViewHor = new KAction( i18n( "Split View &Horizontally" ), CTRL+Key_H, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Vertically" ), CTRL+Key_V, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "Split Window Horizontally" ), 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "Split Window Vertically" ), 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "Remove Active View" ), CTRL+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  m_paSaveDefaultProfile = new KAction( i18n( "Save Current Profile As Default" ), 0, this, SLOT( slotSaveDefaultProfile() ), actionCollection(), "savedefaultprofile" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "Save/Remove View Profile" ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  QPixmap konqpix = KGlobal::iconLoader()->loadIcon( "konqueror", KIconLoader::Small );
  m_paAbout = new KAction( i18n( "&About Konqueror..." ), konqpix, 0, this, SLOT( slotAbout() ), actionCollection(), "about" );
  m_paReportBug = new KAction( i18n( "&Report bug..." ), QIconSet( BarIcon( "pencil", KonqFactory::instance() ) ), 0, this, SLOT( slotReportBug() ), actionCollection(), "reportbug" );

  m_paReload = new KAction( i18n( "&Reload" ), QIconSet( BarIcon( "reload", KonqFactory::instance() ) ), Key_F5, this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paCut = KStdAction::cut( this, SLOT( slotCut() ), actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( this, SLOT( slotCopy() ), actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( this, SLOT( slotPaste() ), actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), QIconSet( BarIcon( "stop", KonqFactory::instance() ) ), Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  m_paTrash = new KAction( i18n( "&Move to Trash" ), QIconSet( BarIcon( "trash", KonqFactory::instance() ) ), 0, this, SLOT( slotTrash() ), actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), CTRL+Key_Delete, this, SLOT( slotDelete() ), actionCollection(), "delete" );

  m_paCut->plug( m_pamEdit->popupMenu() );
  m_paCopy->plug( m_pamEdit->popupMenu() );
  m_paPaste->plug( m_pamEdit->popupMenu() );
  m_paTrash->plug( m_pamEdit->popupMenu() );
  m_paDelete->plug( m_pamEdit->popupMenu() );

  m_pamEdit->popupMenu()->insertSeparator();

  m_paAnimatedLogo = new KAction( QString::null, QIconSet( *s_plstAnimatedLogo->at( 0 ) ), 0, this, SLOT( slotNewWindow() ), actionCollection(), "animated_logo" );

  m_paURLCombo = new KonqComboAction( i18n( "Location " ), 0, this, SLOT( slotURLEntered( const QString & ) ), actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( plugged() ),
           this, SLOT( slotComboPlugged() ) );

  m_paBookmarkBar = new KonqBookmarkBar( "BookmarkBar", 0, this, actionCollection(), "toolbar_bookmark" );

  m_paReload->plug( popup );
  m_paStop->plug( popup );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), actionCollection(), "bookmarks_menu" );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_pamBookmarks->popupMenu(), actionCollection(), true );

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

void KonqMainView::plugInViewGUI( BrowserView *view )
{
  KToolBar *bar = shell()->viewToolBar( "mainToolBar" );

  const QValueList<BrowserView::ViewAction> *actions = view->actions();
  QValueList<BrowserView::ViewAction>::ConstIterator it = actions->begin();
  QValueList<BrowserView::ViewAction>::ConstIterator end = actions->end();
  for (; it != end; ++it )
  {

    // The View Menu is handled in the aboutToShow() slot of the view menu

    if ( ( (*it).m_flags & BrowserView::MenuEdit ) == BrowserView::MenuEdit )
     (*it).m_action->plug( m_pamEdit->popupMenu() );

    if ( ( (*it).m_flags & BrowserView::ToolBar ) == BrowserView::ToolBar &&
         bar )
     (*it).m_action->plug( bar );

  }

  updateExtensionDependendActions( view );
}

void KonqMainView::unPlugViewGUI( BrowserView *view )
{
  KToolBar *bar = shell()->viewToolBar( "mainToolBar" );

  const QValueList<BrowserView::ViewAction> *actions = view->actions();
  QValueList<BrowserView::ViewAction>::ConstIterator it = actions->begin();
  QValueList<BrowserView::ViewAction>::ConstIterator end = actions->end();

  for (; it != end; ++it )
  {

    if ( ( (*it).m_flags & BrowserView::MenuView ) == BrowserView::MenuView )
     (*it).m_action->unplug( m_pamView->popupMenu() );

    if ( ( (*it).m_flags & BrowserView::MenuEdit ) == BrowserView::MenuEdit )
     (*it).m_action->unplug( m_pamEdit->popupMenu() );

    if ( ( (*it).m_flags & BrowserView::ToolBar ) == BrowserView::ToolBar &&
         bar )
     (*it).m_action->unplug( bar );

  }
}

void KonqMainView::updateStatusBar()
{
  if ( !m_progressBar || !m_statusBar )
    return;

  int progress = m_currentView->progress();

  if ( progress != -1 && !m_progressBar->isVisible() )
    m_progressBar->show();
  else
    m_progressBar->hide();

  m_progressBar->setValue( progress );

  m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );

  m_statusBar->changeItem( 0L, STATUSBAR_MSG_ID );
}

void KonqMainView::updateToolBarActions()
{
  // Enables/disables actions that depend on the current view (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  checkEditExtension();

  if ( m_currentView->isLoading() )
    startAnimation(); // takes care of m_paStop
  else
    stopAnimation(); // takes care of m_paStop
}

void KonqMainView::updateExtensionDependendActions( BrowserView *view )
{
  m_paPrint->setEnabled( view->child( 0L, "PrintingExtension" ) != (QObject *)0L );

  bool bViewPropExt = view->child( 0L, "ViewPropertiesExtension" ) != (QObject *)0L;
  m_paSaveSettings->setEnabled( bViewPropExt );
  m_paSaveSettingsPerURL->setEnabled( bViewPropExt );
  checkEditExtension();
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

void KonqMainView::enableAllActions( bool enable )
{
  int count = actionCollection()->count();
  for ( int i = 0; i < count; i++ )
    actionCollection()->action( i )->setEnabled( enable );

  if ( enable )
  {
    if ( ! m_currentView )
      qDebug("No current view !");
    else
      updateExtensionDependendActions( m_currentView->view() );
  }
}

void KonqMainView::openBookmarkURL( const QString & url )
{
  kdebug(0, 1202, "%s", QString("KonqMainView::openBookmarkURL(%1)").arg(url).latin1() );
  openURL( 0L, url );
}

QString KonqMainView::currentTitle()
{
/*
  QString title = caption();

  if ( title.right( 12 ) == " - Konqueror" )
    title.truncate( title.length() - 12 );

  return title;
*/
#ifdef __GNUC__
#warning FIXME
#endif
  return m_currentView->view()->url();
}

QString KonqMainView::currentURL()
{
  return m_currentView->view()->url();
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  m_oldView = m_currentView;

  m_currentView = childView( (BrowserView *)sender() );

  //kdebug(KDEBUG_INFO, 1202, "KonqMainView::slotPopupMenu(...)");
  QString url = m_currentView->url();

  QActionCollection popupMenuCollection;
  if ( !shell()->menuBar()->isVisible() )
    // Somewhat a hack...
    popupMenuCollection.insert( ((KonqShell *) shell())->menuBarAction() );
  popupMenuCollection.insert( m_paBack );
  popupMenuCollection.insert( m_paForward );
  popupMenuCollection.insert( m_paUp );

  checkEditExtension();

  popupMenuCollection.insert( m_paCut );
  popupMenuCollection.insert( m_paCopy );
  popupMenuCollection.insert( m_paPaste );
  popupMenuCollection.insert( m_paTrash );
  popupMenuCollection.insert( m_paDelete );

  KonqPopupMenu * pPopupMenu;
  pPopupMenu = new KonqPopupMenu( _items,
                                 url,
                                 popupMenuCollection,
                                 m_pMenuNew );

  pPopupMenu->exec( _global );
  delete pPopupMenu;

  m_currentView = m_oldView;
  checkEditExtension();
}

void KonqMainView::slotDatabaseChanged()
{
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    QObject *obj = (*it)->view()->child( 0L, "ViewPropertiesExtension" );
    if ( obj )
      ((ViewPropertiesExtension *)obj)->refreshMimeTypes();
  }
}

void KonqMainView::reparseConfiguration()
{
  debug("KonqMainView::reparseConfiguration() !");
  kapp->config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();
  KonqHTMLSettings::reparseConfiguration();
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    QObject *obj = (*it)->view()->child( 0L, "ViewPropertiesExtension" );
    if ( obj )
      ((ViewPropertiesExtension *)obj)->reparseConfiguration();
  }
}

#include "konq_mainview.moc"
