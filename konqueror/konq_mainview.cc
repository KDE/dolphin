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

#include "konq_mainview.h"
#include "konq_shell.h"
#include "konq_part.h"
#include "konq_childview.h"
#include "konq_run.h"
#include "browser.h"
#include "konq_factory.h"
#include "konq_viewmgr.h"
#include "konq_frame.h"
#include "konq_events.h"
#include "konq_iconview.h"
#include "konq_actions.h"

#include <pwd.h>
#include <assert.h>
#include <stdlib.h>

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
#include <kurl.h>
#include <kwm.h>
#include <klineeditdlg.h>
#include <konqdefaults.h>
#include <konqpopupmenu.h>
#include <kprogress.h>
#include <kio_job.h>
#include <kuserpaths.h>
#include <ktrader.h>
#include <ksharedptr.h>
#include <kservice.h>
#include <kpixmapcache.h>

#include <klibloader.h>
#include <part.h>

#include <iostream.h>

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

QList<QPixmap> *KonqMainView::s_plstAnimatedLogo = 0L;

KonqMainView::KonqMainView( KonqPart *part, QWidget *parent, const char *name )
 : View( part, parent, name )
{
  m_currentView = 0L;
  m_pBookmarkMenu = 0L;

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
  
  initActions();
  initPlugins();

  m_bMenuEditDirty = true;
  m_bMenuViewDirty = true;

  connect( QApplication::clipboard(), SIGNAL( dataChanged() ),
           this, SLOT( checkEditExtension() ) );

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Settings" );
  QStringList locationBarCombo = config->readListEntry( "ToolBarCombo" );
  if ( locationBarCombo.count() == 0 )
    locationBarCombo << QString();
  
  m_paURLCombo->setItems( locationBarCombo );
}

KonqMainView::~KonqMainView()
{

  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Settings" );
  config->writeEntry( "ToolBarCombo", m_paURLCombo->comboItems() );
  config->sync();

  m_animatedLogoTimer.stop();
  delete m_pViewManager;

  if ( m_pBookmarkMenu )
    delete m_pBookmarkMenu;
}

void KonqMainView::openURL( KonqChildView *_view, const QString &_url, bool reload, int xOffset,
                            int yOffset )
{
  debug(QString("KonqMainView::openURL : _url = '%1'").arg(_url));

  /////////// First, modify the URL if necessary (adding protocol, ...) //////

  QString url = _url;

  // Root directory?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
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
	return;
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

    kdebug( KDEBUG_INFO, 1202, QString("view->run for %1").arg(url) );
    view->run( url );
    setLocationBarURL( view, url );
    view->setMiscURLData( reload, xOffset, yOffset );
  }
  else
  {
    kdebug( KDEBUG_INFO, 1202, QString("Creating new konqrun for %1").arg(url) );
    (void) new KonqRun( this, 0L, url, 0, false, true );
  }
}

void KonqMainView::openURL( const QString &url, bool reload, int xOffset,
                            int yOffset )
{
  openURL( 0L, url, reload, xOffset, yOffset );
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
    openURL( (KonqChildView *)m_currentView, u.ascii() );
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

void KonqMainView::slotLargeIcons()
{
  if ( m_currentView->view()->inherits( "KonqKfmIconView" ) )
  {
    KonqKfmIconView *iconView = (KonqKfmIconView *)m_currentView->view();
    iconView->setViewMode( Konqueror::LargeIcons );
  }
  else
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::LargeIcons );
  }

  m_bMenuViewDirty = true;
}

void KonqMainView::slotSmallIcons()
{
  if ( m_currentView->view()->inherits( "KonqKfmIconView" ) )
  {
    KonqKfmIconView *iconView = (KonqKfmIconView *)m_currentView->view();
    iconView->setViewMode( Konqueror::SmallIcons );
  }
  else
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::SmallIcons );
  }

  m_bMenuViewDirty = true;
}

void KonqMainView::slotSmallVerticalIcons()
{
  if ( m_currentView->view()->inherits( "KonqKfmIconView" ) )
  {
    KonqKfmIconView *iconView = (KonqKfmIconView *)m_currentView->view();
    iconView->setViewMode( Konqueror::SmallVerticalIcons );
  }
  else
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::SmallVerticalIcons );
  }

  m_bMenuViewDirty = true;
}

void KonqMainView::slotTreeView()
{
  if ( !m_currentView->view()->inherits( "KonqTreeView" ) )
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::TreeView );
  }

  m_bMenuViewDirty = true;
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

void KonqMainView::slotStop()
{
  if ( m_currentView && m_currentView->isLoading() )
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
  u.cd( ".." );

  openURL( (KonqChildView *)m_currentView, u.url() );
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
  openURL( 0L, "~" );
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

  MapViews::ConstIterator it = m_mapViews.find( view );

  assert( it != m_mapViews.end() );

  (*it)->setLoading( true );

  (*it)->makeHistory( true );
  
  if ( (KonqChildView *)m_currentView == *it )
  {
    startAnimation();
    setUpEnabled( url );
    m_paBack->setEnabled( m_currentView->canGoBack() );
    m_paForward->setEnabled( m_currentView->canGoForward() );

    updateStatusBar();
  }

}

void KonqMainView::slotCompleted()
{
  BrowserView *view = (BrowserView *)sender();

  MapViews::ConstIterator it = m_mapViews.find( view );

  (*it)->setLoading( false );
  (*it)->setProgress( -1 );

  if ( (KonqChildView *)m_currentView == *it )
  {
    stopAnimation();
    m_paBack->setEnabled( m_currentView->canGoBack() );
    m_paForward->setEnabled( m_currentView->canGoForward() );

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
}

void KonqMainView::slotSetStatusBarText( const QString &text )
{
  if ( !m_statusBar )
    return;

  m_statusBar->changeItem( text, STATUSBAR_MSG_ID );
}

bool KonqMainView::openView( const QString &serviceType, const QString &url, KonqChildView *childView )
{
  QString indexFile;

  if ( !childView )
    {
      QStringList serviceTypes;
      BrowserView *view = KonqFactory::createView( serviceType, serviceTypes );
      assert(view != 0);
      m_pViewManager->splitView( Qt::Horizontal, view, serviceTypes );

      MapViews::Iterator it = m_mapViews.find( view );
      assert( it != m_mapViews.end() );

      it.data()->openURL( url, true );

      setActiveView( view );

      return true;
    }
  else
  {
    debug(QString("(1) KonqMainView::openView : url = '%1'").arg(url));
    childView->stop();
  }
  debug(QString("(2) KonqMainView::openView : url = '%1'").arg(url));

  //first check whether the current view can display this type directly, then
  //try to change the view mode. if this fails, too, then Konqueror cannot
  //display the data addressed by the URL
  if ( childView->supportsServiceType( serviceType ) )
  {
    KURL u( url );
    if ( ( serviceType == "inode/directory" ) &&
         ( childView->allowHTML() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
      childView->changeViewMode( "text/html", indexFile.prepend( "file:" ) );
    else
    {
      childView->makeHistory( false );
      childView->openURL( url, true );
    }

    return true;
  }

  if ( childView->changeViewMode( serviceType, url ) )
    return true;

  return false;
}

void KonqMainView::setActiveView( BrowserView *view )
{
  KonqChildView *oldView = m_currentView;

  if ( m_currentView )
    unPlugViewGUI( m_currentView->view() );

  m_currentView = m_mapViews.find( view ).data();

  m_currentView->frame()->header()->repaint();
  
  if ( oldView )
    oldView->frame()->header()->repaint();

  plugInViewGUI( view );

  m_paURLCombo->changeItem( m_currentView->locationBarURL(), 0 );

  updateStatusBar();
  
  m_paStop->setEnabled( m_currentView->isLoading() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  checkEditExtension();
  
  m_bMenuViewDirty = true;
}

void KonqMainView::insertChildView( KonqChildView *childView )
{
  m_mapViews.insert( childView->view(), childView );

  m_paRemoveView->setEnabled( m_mapViews.count() > 1 );
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

    m_paURLCombo->setCurrentItem( 0 );
    m_paURLCombo->changeItem( url, 0 );
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
  m_paURLCombo->blockSignals( true );
  KonqURLEnterEvent ev( text );
  QApplication::sendEvent( this, &ev );
  m_paURLCombo->blockSignals( false );
}

void KonqMainView::slotFileNewAboutToShow()
{
  // As requested by KNewMenu :
  m_pMenuNew->slotCheckUpToDate();
  // And set the files that the menu apply on :
  QStringList urls;
  urls.append( m_currentView->url() );
  m_pMenuNew->setPopupFiles( urls );
}

void KonqMainView::slotMenuEditAboutToShow()
{
  if ( !m_bMenuEditDirty )
    return;

  m_bMenuEditDirty = false;
}

void KonqMainView::slotMenuViewAboutToShow()
{
  if ( !m_bMenuViewDirty )
    return;

  //eeek! (Simon)
  m_ptaLargeIcons->blockSignals( true );
  m_ptaSmallIcons->blockSignals( true );
  m_ptaSmallVerticalIcons->blockSignals( true );
  m_ptaTreeView->blockSignals( true );
  m_ptaUseHTML->blockSignals( true );

  m_ptaLargeIcons->setChecked( false );
  m_ptaSmallIcons->setChecked( false );
  m_ptaSmallVerticalIcons->setChecked( false );
  m_ptaTreeView->setChecked( false );

  m_ptaUseHTML->setChecked( m_currentView->allowHTML() );

  BrowserView *view = m_currentView->view();

  if ( view->inherits( "KonqKfmIconView" ) )
  {
    Konqueror::DirectoryDisplayMode dirMode = ((KonqKfmIconView *) view)->viewMode();

    switch ( dirMode )
    {
      case Konqueror::LargeIcons:
        m_ptaLargeIcons->setChecked( true );
	break;
      case Konqueror::SmallIcons:
        m_ptaSmallIcons->setChecked( true );
	break;
      case Konqueror::SmallVerticalIcons:
        m_ptaSmallVerticalIcons->setChecked( true );
	break;
      default: assert( 0 );
    }

  }
  else if ( view->inherits( "KonqTreeView" ) )
    m_ptaTreeView->setChecked( true );

  m_ptaLargeIcons->blockSignals( false );
  m_ptaSmallIcons->blockSignals( false );
  m_ptaSmallVerticalIcons->blockSignals( false );
  m_ptaTreeView->blockSignals( false );
  m_ptaUseHTML->blockSignals( false );

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
  //TODO
}

void KonqMainView::slotSplitWindowVertical()
{
  //TODO
}

void KonqMainView::slotRemoveView()
{
  KonqChildView *nextView = m_pViewManager->chooseNextView( (KonqChildView *)m_currentView );
  KonqChildView *prevView = m_currentView;

  setActiveView( nextView->view() );

  m_pViewManager->removeView( prevView );
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

  if ( m_currentView->view() == view && m_currentView->isLoading() )
  {
    if ( !m_progressBar->isVisible() )
      m_progressBar->show();

    m_progressBar->setValue( percent );
  }

  childView( view )->setProgress( percent );
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
  bool bCopy = false;
  bool bPaste = false;
  bool bMove = false;

  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->can( bCopy, bPaste, bMove );

  m_paCopy->setEnabled( bCopy );
  m_paPaste->setEnabled( bPaste );
  m_paTrash->setEnabled( bMove );
  m_paDelete->setEnabled( bMove );
}

void KonqMainView::slotCopy()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->copySelection();
}

void KonqMainView::slotPaste()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->pasteSelection();
}

void KonqMainView::slotTrash()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
    ((EditExtension *)obj)->moveSelection( KUserPaths::trashPath().utf8() );
}

void KonqMainView::slotDelete()
{
  QObject *obj = m_currentView->view()->child( 0L, "EditExtension" );
  if ( obj )
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
"Konqueror Version 0.1\n"
"Author: Torben Weis <weis@kde.org>\n"
"Current maintainer: David Faure <faure@kde.org>\n\n"
"Current team:\n"
"David Faure <faure@kde.org>\n"
"Simon Hausmann <hausmann@kde.org>\n"
"Michael Reiher <michael.reiher@gmx.de>\n"
"Matthias Welk <welk@fokus.gmd.de>\n"
"Waldo Bastian <bastian@kde.org>, Lars Knoll <knoll@kde.org> (khtml library)\n"
"Matt Koss <koss@napri.sk>, Alex Zepeda <garbanzo@hooked.net> (kio library/slaves)\n"
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
    popup->insertItem( u.decodedURL() );

    if ( u.path() == "/" )
      break;

    if ( ++i > 10 )
      break;

    u.cd( ".." );
  }
}

void KonqMainView::slotBackAboutToShow()
{
  fillHistoryPopup( m_paBack->popupMenu(), m_currentView->backHistoryURLs() );
}

void KonqMainView::slotForwardAboutToShow()
{
  fillHistoryPopup( m_paForward->popupMenu(), m_currentView->forwardHistoryURLs() );
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

void KonqMainView::fillHistoryPopup( QPopupMenu *menu, const QStringList &urls )
{
  menu->clear();

  QStringList::ConstIterator it = urls.begin();
  QStringList::ConstIterator end = urls.end();
  uint i = 0;
  for (; it != end; ++it )
  {
    KURL u( *it );
    menu->insertItem( *KPixmapCache::pixmapForURL( u, 0, u.isLocalFile(), true ),
                      *it );
    if ( ++i > 10 )
      break;
  }

}

void KonqMainView::setLocationBarURL( KonqChildView *childView, const QString &url )
{
  childView->setLocationBarURL( url );
  
  if ( childView == (KonqChildView *)m_currentView )
    m_paURLCombo->changeItem( url, 0 );

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

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainView::initActions()
{
  KStdAccel stdAccel;

  m_pMenuNew = new KNewMenu ( actionCollection(), "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paNewWindow = new KAction( i18n( "New &Window" ), stdAccel.openNew(), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );

  m_paRun = new KAction( i18n( "&Run..." ), 0, this, SLOT( slotRun() ), actionCollection(), "run" );
  m_paOpenTerminal = new KAction( i18n( "Open &Terminal..." ), CTRL+Key_T, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  m_paOpenLocation = new KAction( i18n( "Open &Location..." ), CTRL+Key_L, this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );
  m_paToolFind = new KAction( i18n( "&Find" ), 0, this, SLOT( slotToolFind() ), actionCollection(), "find" );

  m_paPrint = new KAction( i18n( "&Print..."), QIconSet( BarIcon( "fileprint", KonqFactory::instance() ) ), stdAccel.print(), this, SLOT( slotPrint() ), actionCollection(), "print" );

  // Edit menu
  m_pamEdit = new KActionMenu( i18n( "&Edit" ), actionCollection(), "edit_menu" );

  connect( m_pamEdit->popupMenu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotMenuEditAboutToShow() ) );

  // View menu
  m_pamView = new KActionMenu( i18n( "&View" ), actionCollection(), "view_menu" );

  QPopupMenu *popup = m_pamView->popupMenu();

  connect( popup, SIGNAL( aboutToShow() ),
           this, SLOT( slotMenuViewAboutToShow() ) );

  // these actions don't actually need to be children of actioncollection, but
  // OTOH this deletes them automatically upon destruction (Simon)
  m_ptaLargeIcons = new KToggleAction( i18n( "&Large Icons" ), 0, this, SLOT( slotLargeIcons() ), actionCollection(), "largeicons" );
  m_ptaSmallIcons = new KToggleAction( i18n( "&Small Icons" ), 0, this, SLOT( slotSmallIcons() ), actionCollection(), "smallicons" );
  m_ptaSmallVerticalIcons = new KToggleAction( i18n( "Small Vertical Icons" ), 0, this, SLOT( slotSmallVerticalIcons() ), actionCollection(), "smallverticalicons" );
  m_ptaTreeView = new KToggleAction( i18n( "&Tree View" ), 0, this, SLOT( slotTreeView() ), actionCollection(), "treeview" );

  m_ptaLargeIcons->plug( popup );
  m_ptaSmallIcons->plug( popup );
  m_ptaSmallVerticalIcons->plug( popup );
  m_ptaTreeView->plug( popup );

  popup->insertSeparator();

  m_ptaUseHTML = new KToggleAction( i18n( "&Use HTML" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );

  m_ptaUseHTML->plug( popup );

  popup->insertSeparator();

  m_paUp = new KActionMenu( i18n( "&Up" ), QIconSet( BarIcon( "up", KonqFactory::instance() ) ), actionCollection(), "up" );

  connect( m_paUp, SIGNAL( activated() ), this, SLOT( slotUp() ) );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  m_paBack = new KActionMenu( i18n( "&Back" ), QIconSet( BarIcon( "back", KonqFactory::instance() ) ), actionCollection(), "back" );

  m_paBack->setEnabled( false );

  connect( m_paBack, SIGNAL( activated() ), this, SLOT( slotBack() ) );
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KActionMenu( i18n( "&Forward" ), QIconSet( BarIcon( "forward", KonqFactory::instance() ) ), actionCollection(), "forward" );

  m_paForward->setEnabled( false );

  connect( m_paForward, SIGNAL( activated() ), this, SLOT( slotForward() ) );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHome = new KAction( i18n("&Home" ), QIconSet( BarIcon( "home", KonqFactory::instance() ) ), 0, this, SLOT( slotHome() ), actionCollection(), "home" );

  m_paCache = new KAction( i18n( "&Cache" ), 0, this, SLOT( slotShowCache() ), actionCollection(), "cache" );
  m_paHistory = new KAction( i18n( "&History" ), 0, this, SLOT( slotShowHistory() ), actionCollection(), "history" );
  m_paMimeTypes = new KAction( i18n( "Mime &Types" ), 0, this, SLOT( slotEditMimeTypes() ), actionCollection(), "mimetypes" );
  m_paApplications = new KAction( i18n( "App&lications" ), 0, this, SLOT( slotEditApplications() ), actionCollection(), "applications" );

  m_paShowMenuBar = new KAction( i18n( "Show &Menubar" ), 0, this, SLOT( slotShowMenuBar() ), actionCollection(), "showmenubar" );
  m_paShowStatusBar = new KAction( i18n( "Show &Statusbar" ), 0, this, SLOT( slotShowStatusBar() ), actionCollection(), "showstatusbar" );
  m_paShowToolBar = new KAction( i18n( "Show &Toolbar" ), 0, this, SLOT( slotShowToolBar() ), actionCollection(), "showtoolbar" );
  m_paShowLocationBar = new KAction( i18n( "Show &Locationbar" ), 0, this, SLOT( slotShowLocationBar() ), actionCollection(), "showlocationbar" );

  m_paSaveSettings = new KAction( i18n( "Sa&ve Settings" ), 0, this, SLOT( slotSaveSettings() ), actionCollection(), "savesettings" );
  m_paSaveSettingsPerURL = new KAction( i18n( "Save Settings for this &URL" ), 0, this, SLOT( slotSaveSettingsPerURL() ), actionCollection(), "savesettingsperurl" );

  m_paConfigureFileManager = new KAction( i18n( "&Configure File Manager..." ), 0, this, SLOT( slotConfigureFileManager() ), actionCollection(), "configurefilemanager" );
  m_paConfigureBrowser = new KAction( i18n( "Configure &Browser..." ), 0, this, SLOT( slotConfigureBrowser() ), actionCollection(), "configurebrowser" );
  m_paConfigureKeys = new KAction( i18n( "Configure &keys..." ), 0, this, SLOT( slotConfigureKeys() ), actionCollection(), "configurekeys" );
  m_paReloadPlugins = new KAction( i18n( "Reload Plugins" ), 0, this, SLOT( slotReloadPlugins() ), actionCollection(), "reloadplugins" );
  m_paConfigurePlugins = new KAction( i18n( "Configure Plugins..." ), 0, this, SLOT( slotConfigurePlugins() ), actionCollection(), "configureplugins" );

  m_paSplitViewHor = new KAction( i18n( "Split View &Horizontally" ), 0, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Vertically" ), 0, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paSplitWindowHor = new KAction( i18n( "Split Window &Horizontally" ), 0, this, SLOT( slotSplitWindowHorizontal() ), actionCollection(), "splitwindowh" );
  m_paSplitWindowVer = new KAction( i18n( "Split Window &Vertically" ), 0, this, SLOT( slotSplitWindowVertical() ), actionCollection(), "splitwindowv" );
  m_paRemoveView = new KAction( i18n( "Remove Active View" ), CTRL+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );

  m_paSaveDefaultProfile = new KAction( i18n( "Save Current Profile As Default" ), 0, this, SLOT( slotSaveDefaultProfile() ), actionCollection(), "savedefaultprofile" );

  m_paSaveRemoveViewProfile = new KAction( i18n( "Save/Remove View Profile" ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load View Profile" ), actionCollection(), "loadviewprofile" );
  
  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_paAbout = new KAction( i18n( "&About Konqueror" ), 0, this, SLOT( slotAbout() ), actionCollection(), "about" );

  m_paReload = new KAction( i18n( "&Reload Document" ), QIconSet( BarIcon( "reload", KonqFactory::instance() ) ), Key_F5, this, SLOT( slotReload() ), actionCollection(), "reload" );

  m_paCopy = new KAction( i18n( "&Copy" ), QIconSet( BarIcon( "editcopy", KonqFactory::instance() ) ), stdAccel.copy(), this, SLOT( slotCopy() ), actionCollection(), "copy" );
  m_paPaste = new KAction( i18n( "&Paste" ), QIconSet( BarIcon( "editpaste", KonqFactory::instance() ) ), stdAccel.paste(), this, SLOT( slotPaste() ), actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "Sto&p loading" ), QIconSet( BarIcon( "stop", KonqFactory::instance() ) ), Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  m_paTrash = new KAction( i18n( "&Move to Trash" ), QIconSet( BarIcon( "trash", KonqFactory::instance() ) ), stdAccel.cut(), this, SLOT( slotTrash() ), actionCollection(), "trash" );
  m_paDelete = new KAction( i18n( "&Delete" ), CTRL+Key_Delete, this, SLOT( slotDelete() ), actionCollection(), "delete" );

  m_paCopy->plug( m_pamEdit->popupMenu() );
  m_paPaste->plug( m_pamEdit->popupMenu() );
  m_paTrash->plug( m_pamEdit->popupMenu() );
  m_paDelete->plug( m_pamEdit->popupMenu() );

  m_pamEdit->popupMenu()->insertSeparator();

  m_paAnimatedLogo = new KAction( QString::null, QIconSet( *s_plstAnimatedLogo->at( 0 ) ), 0, this, SLOT( slotNewWindow() ), actionCollection(), "animated_logo" );

  m_paURLCombo = new KonqComboAction( i18n( "Location " ), 0, actionCollection(), "toolbar_url_combo" );
  connect( m_paURLCombo, SIGNAL( activated( const QString & ) ),
           this, SLOT( slotURLEntered( const QString & ) ) );

  m_paURLCombo->setEditable( true );

  m_paReload->plug( popup );
  m_paStop->plug( popup );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), actionCollection(), "bookmarks_menu" );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_pamBookmarks->popupMenu(), actionCollection(), true );
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

    if ( ( (*it).m_flags & BrowserView::MenuView ) == BrowserView::MenuView )
     (*it).m_action->plug( m_pamView->popupMenu() );

    if ( ( (*it).m_flags & BrowserView::MenuEdit ) == BrowserView::MenuEdit )
     (*it).m_action->plug( m_pamEdit->popupMenu() );

    if ( ( (*it).m_flags & BrowserView::ToolBar ) == BrowserView::ToolBar &&
         bar )
     (*it).m_action->plug( bar );

  }

  m_paPrint->setEnabled( view->child( 0L, "PrintingExtension" ) != (QObject *)0L );
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

void KonqMainView::openBookmarkURL( const QString & url )
{
  kdebug(0,1202,QString("KonqMainView::openBookmarkURL(%1)").arg(url));
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
#warning FIXME
  return m_currentView->view()->url();  
}

QString KonqMainView::currentURL()
{
  return m_currentView->view()->url();
}

void KonqMainView::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  kdebug(KDEBUG_INFO, 1202, "KonqMainView::slotPopupMenu(...)");
  QString url = m_currentView->url();
  // TODO enable "back" if m_currentView->canGoBack();
  // TODO enable "forward" if m_currentView->canGoForward();
  // Well, not here, but dynamically, so that the go menu is right as well

  QActionCollection popupMenuCollection;
  // if ( !m_vMenuBar->isVisible() )
  // popupMenuCollection.insert( m_paShowMenuBar );
  popupMenuCollection.insert( m_paBack );
  popupMenuCollection.insert( m_paForward );
  popupMenuCollection.insert( m_paUp );
  // TODO : if support edit extension ...
  popupMenuCollection.insert( m_paCopy );
  popupMenuCollection.insert( m_paPaste );
  popupMenuCollection.insert( m_paTrash );
  // TODO : delete ...

  KonqPopupMenu * pPopupMenu;
  pPopupMenu = new KonqPopupMenu( _items,
                                 url,
                                 popupMenuCollection,
                                 m_pMenuNew );

  pPopupMenu->exec( _global );
  delete pPopupMenu;
}

#include "konq_mainview.moc"
