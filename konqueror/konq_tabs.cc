/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers
                  2002-2003 Douglas Hanley <douglash@caltech.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA  02111-1307, USA.
*/

#include "konq_tabs.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qptrlist.h>
#include <qpopupmenu.h>
#include <qtoolbutton.h>
#include <qtooltip.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurldrag.h>

#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"
#include "konq_misc.h"

#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
#include <qtabbar.h>
#include <qstyle.h>

#define BREAKOFF_ID 5
#define CLOSETAB_ID 6
#define RELOAD_ALL_ID 7
#define CLOSE_OTHER_ID 8

//###################################################################

KonqFrameTabs::KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
                             KonqViewManager* viewManager, const char * name)
  : KTabWidget(parent, name), m_CurrentMaxLength(30), m_rightWidget(0)
{
  //kdDebug(1202) << "KonqFrameTabs::KonqFrameTabs()" << endl;

  m_pParentContainer = parentContainer;
  m_pChildFrameList = new QPtrList<KonqFrameBase>;
  m_pChildFrameList->setAutoDelete(false);
  m_pActiveChild = 0L;
  m_pViewManager = viewManager;

  connect( this, SIGNAL( currentChanged ( QWidget * ) ),
           this, SLOT( slotCurrentChanged( QWidget* ) ) );

  m_pPopupMenu = new QPopupMenu( this );
  m_pPopupMenu->insertItem( SmallIcon( "tab_new" ),
                            i18n("&New Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotAddTab() ),
                            m_pViewManager->mainWindow()->action("newtab")->shortcut() );
  m_pPopupMenu->insertItem( SmallIcon( "tab_duplicate" ),
                            i18n("&Duplicate Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotDuplicateTabPopup() ),
                            m_pViewManager->mainWindow()->action("duplicatecurrenttab")->shortcut() );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_breakoff" ),
                            i18n("D&etach Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotBreakOffTabPopup() ),
                            m_pViewManager->mainWindow()->action("breakoffcurrenttab")->shortcut(),
                            BREAKOFF_ID );
  m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ),
			    i18n("&Close Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotRemoveTabPopup() ),
                            m_pViewManager->mainWindow()->action("removecurrenttab")->shortcut(),
                            CLOSETAB_ID );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "reload" ),
                            i18n( "&Reload" ),
                            m_pViewManager->mainWindow(),
                            SLOT( slotReloadPopup() ),
                            KStdAccel::shortcut(KStdAccel::Reload) );
  m_pPopupMenu->insertItem( SmallIcon( "reload_all_tabs" ),
                            i18n( "&Reload All Tabs" ),
                            m_pViewManager->mainWindow(),
                            SLOT( slotReloadAllTabs() ),
                            QKeySequence(),
                            RELOAD_ALL_ID );
  m_pPopupMenu->insertSeparator();
  m_pSubPopupMenuTab = new QPopupMenu( this );
  m_pPopupMenu->insertItem( i18n("Switch to Tab" ), m_pSubPopupMenuTab );
  connect( m_pSubPopupMenuTab, SIGNAL( activated ( int ) ),
           this, SLOT( slotSubPopupMenuTabActivated( int ) ) );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ),
                            i18n( "Close &Other Tabs" ),
                            m_pViewManager->mainWindow(),
                            SLOT( slotRemoveOtherTabsPopup() ),
                            QKeySequence(),
                            CLOSE_OTHER_ID );
  connect( this, SIGNAL( contextMenu( QWidget *, const QPoint & ) ),
           SLOT(slotContextMenu( QWidget *, const QPoint & ) ) );

  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
  m_permanentCloseButtons = config->readBoolEntry( "PermanentCloseButton", false );
  if (m_permanentCloseButtons) {
    setHoverCloseButton( true );
    setHoverCloseButtonDelayed( false );
  }
  else
    setHoverCloseButton( config->readBoolEntry( "HoverCloseButton", false ) );
  connect( this, SIGNAL( closeRequest( QWidget * )), SLOT(slotCloseRequest( QWidget * )));
  connect( this, SIGNAL( removeTabPopup() ),
           m_pViewManager->mainWindow(), SLOT( slotRemoveTabPopup() ) );

  if ( config->readBoolEntry( "AddTabButton", true ) ) {
    QToolButton * leftWidget = new QToolButton( this );
    connect( leftWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotAddTab() ) );
    leftWidget->setIconSet( SmallIcon( "tab_new" ) );
    leftWidget->adjustSize();
    QToolTip::add(leftWidget, i18n("Open a new tab"));
    setCornerWidget( leftWidget, TopLeft );
  }
  if ( config->readBoolEntry( "CloseTabButton", true ) ) {
    m_rightWidget = new QToolButton( this );
    connect( m_rightWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotRemoveTab() ) );
    m_rightWidget->setIconSet( SmallIcon( "tab_remove" ) );
    m_rightWidget->adjustSize();
    QToolTip::add(m_rightWidget, i18n("Close the current tab"));
    setCornerWidget( m_rightWidget, TopRight );
  }

  setTabReorderingEnabled( true );
  connect( this, SIGNAL( movedTab( int, int ) ),
           SLOT( slotMovedTab( int, int ) ) );
  connect( this, SIGNAL( mouseMiddleClick() ),
           SLOT( slotMouseMiddleClick() ) );
  connect( this, SIGNAL( mouseMiddleClick( QWidget * ) ),
           SLOT( slotMouseMiddleClick( QWidget * ) ) );

  connect( this, SIGNAL( testCanDecode(const QDragMoveEvent *, bool & )),
           SLOT( slotTestCanDecode(const QDragMoveEvent *, bool & ) ) );
  connect( this, SIGNAL( receivedDropEvent( QDropEvent * )),
           SLOT( slotReceivedDropEvent( QDropEvent * ) ) );
  connect( this, SIGNAL( receivedDropEvent( QWidget *, QDropEvent * )),
           SLOT( slotReceivedDropEvent( QWidget *, QDropEvent * ) ) );
  connect( this, SIGNAL( initiateDrag( QWidget * )),
           SLOT( slotInitiateDrag( QWidget * ) ) );
}

KonqFrameTabs::~KonqFrameTabs()
{
  //kdDebug(1202) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className() << endl;
  m_pChildFrameList->setAutoDelete(true);
  delete m_pChildFrameList;
}

void KonqFrameTabs::listViews( ChildViewList *viewList ) {
  for( QPtrListIterator<KonqFrameBase> it( *m_pChildFrameList ); *it; ++it )
    it.current()->listViews(viewList);
}

void KonqFrameTabs::saveConfig( KConfig* config, const QString &prefix, bool saveURLs,
                                KonqFrameBase* docContainer, int id, int depth )
{
  //write children
  QStringList strlst;
  int i = 0;
  QString newPrefix;
  for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
    {
      newPrefix = QString::fromLatin1( it->frameType() ) + "T" + QString::number(i);
      strlst.append( newPrefix );
      newPrefix.append( '_' );
      it->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth);
      i++;
    }

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ),
                      currentPageIndex() );
}

void KonqFrameTabs::copyHistory( KonqFrameBase *other )
{
  if( other->frameType() != "Tabs" ) {
    kdDebug(1202) << "Frame types are not the same" << endl;
    return;
  }

  for (uint i = 0; i < m_pChildFrameList->count(); i++ )
    {
      m_pChildFrameList->at(i)->copyHistory( static_cast<KonqFrameTabs *>( other )->m_pChildFrameList->at(i) );
    }
}

void KonqFrameTabs::printFrameInfo( const QString& spaces )
{
  kdDebug(1202) << spaces << "KonqFrameTabs " << this << " visible="
                << QString("%1").arg(isVisible()) << " activeChild="
                << m_pActiveChild << endl;

  if (!m_pActiveChild)
      kdDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;

  KonqFrameBase* child;
  int childFrameCount = m_pChildFrameList->count();
  for (int i = 0 ; i < childFrameCount ; i++) {
    child = m_pChildFrameList->at(i);
    if (child != 0L)
      child->printFrameInfo(spaces + "  ");
    else
      kdDebug(1202) << spaces << "  Null child" << endl;
  }
}

void KonqFrameTabs::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

uint KonqFrameTabs::tabBarWidthForMaxChars( uint maxLength )
{
  int hframe, overlap;
  hframe  = tabBar()->style().pixelMetric( QStyle::PM_TabBarTabHSpace, this );
  overlap = tabBar()->style().pixelMetric( QStyle::PM_TabBarTabOverlap, this );

  QFontMetrics fm = tabBar()->fontMetrics();
  int x = 0;
  for( int i=0; i < count(); ++i ) {
    QString newTitle;
    KonqFrame* konqframe = dynamic_cast<KonqFrame*>( page(i) );
    if ( konqframe )
      newTitle = konqframe->title();
    else {
      KonqView* konqview= static_cast<KonqFrameContainer*>( page(i) )->activeChildView();
      if ( konqview )
        newTitle = konqview->frame()->title();
    }

    if ( newTitle.length() > maxLength )
      newTitle = newTitle.left( maxLength-3 ) + "...";

    QTab* tab = tabBar()->tabAt( i );
    int lw = fm.width( newTitle );
    int iw = 0;
    if ( tab->iconSet() )
      iw = tab->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).width() + 4;
    x += ( tabBar()->style().sizeFromContents( QStyle::CT_TabBarTab, this,
                   QSize( QMAX( lw + hframe + iw, QApplication::globalStrut().width() ), 0 ),
                   QStyleOption( tab ) ) ).width();
  }
  return x;
}

void KonqFrameTabs::setTitle( const QString &title , QWidget* sender)
{
  // kdDebug(1202) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )" << endl;
  removeTabToolTip( sender );

  uint lcw=0, rcw=0;
#if QT_VERSION >= 0x030200
  int tabBarHeight = tabBar()->sizeHint().height();
  if ( cornerWidget( TopLeft ) && cornerWidget( TopLeft )->isVisible() )
    lcw = QMAX( cornerWidget( TopLeft )->width(), tabBarHeight );
  if ( cornerWidget( TopRight ) && cornerWidget( TopRight )->isVisible() )
    rcw = QMAX( cornerWidget( TopRight )->width(), tabBarHeight );
#endif
  uint maxTabBarWidth = width() - lcw - rcw;
  // kdDebug(1202) << "maxTabBarWidth=" << maxTabBarWidth << endl;

  uint newMaxLength=30;
  for ( ; newMaxLength > 3; newMaxLength-- ) {
    // kdDebug(1202) << "tabBarWidthForMaxChars(" << newMaxLength
    //               << ")=" << tabBarWidthForMaxChars( newMaxLength ) << endl;
    if ( tabBarWidthForMaxChars( newMaxLength ) < maxTabBarWidth )
      break;
  }
  // kdDebug(1202) << "newMaxLength=" << newMaxLength << endl;

  QString newTitle = title;
  if ( newTitle.length() > newMaxLength )
    {
      setTabToolTip( sender, newTitle );
      newTitle = newTitle.left( newMaxLength-3 ) + "...";
    }
  newTitle.replace( '&', "&&" );
  if ( tabLabel( sender ) != newTitle )
    changeTab( sender, newTitle );

  if( newMaxLength != m_CurrentMaxLength )
    {
      for( int i = 0; i < count(); ++i)
        {
          KonqFrame* konqframe = dynamic_cast<KonqFrame*>( page(i) );
          if ( konqframe )
            newTitle = konqframe->title();
          else {
            KonqView* konqview= static_cast<KonqFrameContainer*>( page(i) )->activeChildView();
            if ( konqview )
              newTitle = konqview->frame()->title();
          }

          removeTabToolTip( page( i ) );
          if ( newTitle.length() > newMaxLength )
            {
              setTabToolTip( page( i ), newTitle );
              newTitle = newTitle.left( newMaxLength-3 ) + "...";
            }
          newTitle.replace( '&', "&&" );
          if ( newTitle != tabLabel( page( i ) ) )
            changeTab( page( i ), newTitle );
        }
      m_CurrentMaxLength = newMaxLength;
    }
}

void KonqFrameTabs::setTabIcon( const QString &url, QWidget* sender )
{
  //kdDebug(1202) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )" << endl;
  QIconSet iconSet;
  if (m_permanentCloseButtons)
    iconSet =  SmallIcon( "fileclose" );
  else
    iconSet =  QIconSet( KonqPixmapProvider::self()->pixmapFor( url ) );
  if (tabIconSet( sender ).pixmap().serialNumber() != iconSet.pixmap().serialNumber())
    setTabIconSet( sender, iconSet );
}

void KonqFrameTabs::activateChild()
{
  if (m_pActiveChild)
    {
      showPage( m_pActiveChild->widget() );
      m_pActiveChild->activateChild();
    }
}

void KonqFrameTabs::insertChildFrame( KonqFrameBase* frame, int index )
{
  //kdDebug(1202) << "KonqFrameTabs " << this << ": insertChildFrame " << frame << endl;

  if (frame)
    {
      //kdDebug(1202) << "Adding frame" << endl;
      insertTab(frame->widget(),"", index);
      frame->setParentContainer(this);
      if (index == -1) m_pChildFrameList->append(frame);
      else m_pChildFrameList->insert(index, frame);
      if (m_rightWidget)
        m_rightWidget->setEnabled( m_pChildFrameList->count()>1 );
      KonqView* activeChildView = frame->activeChildView();
      if (activeChildView != 0L) {
        activeChildView->setCaption( activeChildView->caption() );
        activeChildView->setTabIcon( activeChildView->url().url() );
      }
    }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameTabs::removeChildFrame( KonqFrameBase * frame )
{
  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if (frame) {
    removePage(frame->widget());
    m_pChildFrameList->remove(frame);
    if (m_rightWidget)
      m_rightWidget->setEnabled( m_pChildFrameList->count()>1 );
  }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": removeChildFrame(0L) !" << endl;

  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame finished" << endl;
}

void KonqFrameTabs::slotCurrentChanged( QWidget* newPage )
{
  setTabColor( newPage, KGlobalSettings::textColor() );
  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>(newPage);

  if (currentFrame && !m_pViewManager->isLoadingProfile()) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::moveTabBackward( int index )
{
  if ( index == 0 )
    return;
  moveTab( index, index-1 );
}

void KonqFrameTabs::moveTabForward( int index )
{
  if ( index == count()-1 )
    return;
  moveTab( index, index+1 );
}

void KonqFrameTabs::slotMovedTab( int from, int to )
{
  KonqFrameBase* fromFrame = m_pChildFrameList->at( from );
  m_pChildFrameList->remove( fromFrame );
  m_pChildFrameList->insert( to, fromFrame );

  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>( currentPage() );
  if ( currentFrame && !m_pViewManager->isLoadingProfile() ) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::slotContextMenu( QWidget *w, const QPoint &p )
{
  m_pPopupMenu->setItemEnabled( BREAKOFF_ID, m_pChildFrameList->count()>1 );
  m_pPopupMenu->setItemEnabled( CLOSETAB_ID, m_pChildFrameList->count()>1 );
  m_pPopupMenu->setItemEnabled( RELOAD_ALL_ID, m_pChildFrameList->count()>1 );
  m_pPopupMenu->setItemEnabled( CLOSE_OTHER_ID, m_pChildFrameList->count()>1 );
  // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
  m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
  refreshSubPopupMenuTab();
  m_pPopupMenu->exec( p );
}

void KonqFrameTabs::refreshSubPopupMenuTab()
{
    m_pSubPopupMenuTab->clear();
    int i=0;
    for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
    {
        KonqFrame* frame = dynamic_cast<KonqFrame *>(it);
        if ( frame && frame->activeChildView() )
        {
            QString title = frame->title().stripWhiteSpace();
            if ( title.isEmpty() )
                title = frame->activeChildView()->url().url();
            m_pSubPopupMenuTab->insertItem( QIconSet( KonqPixmapProvider::self()->pixmapFor( frame->activeChildView()->url().url() ) ), title, i );

        }
        i++;
    }

}

void KonqFrameTabs::slotCloseRequest( QWidget *w )
{
  if ( m_pChildFrameList->count() > 1 ) {
    // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
    m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
    emit ( removeTabPopup() );
  }
}

void KonqFrameTabs::slotSubPopupMenuTabActivated( int _id)
{
    setCurrentPage( _id );
}

void KonqFrameTabs::slotMouseMiddleClick()
{
  QApplication::clipboard()->setSelectionMode( QClipboard::Selection );
  KURL filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text() ) );
  if ( !filteredURL.isEmpty() ) {
    KonqView* newView = m_pViewManager->addTab(QString::null, QString::null, false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openURL( newView, filteredURL, QString::null );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotMouseMiddleClick( QWidget *w )
{
  QApplication::clipboard()->setSelectionMode( QClipboard::Selection );
  KURL filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text() ) );
  if ( !filteredURL.isEmpty() ) {
    KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
    if (frame) {
      m_pViewManager->mainWindow()->openURL( frame->activeChildView(), filteredURL );
    }
  }
}

void KonqFrameTabs::resizeEvent( QResizeEvent *e )
{
    KTabWidget::resizeEvent( e );
    if ( count() ) {
      KonqFrame* konqframe = dynamic_cast<KonqFrame*>( page(0) );
      if ( konqframe )
        setTitle( konqframe->title(), page( 0 ) );
      else {
        KonqView* konqview= static_cast<KonqFrameContainer*>( page(0) )->activeChildView();
        if ( konqview )
          setTitle( konqview->frame()->title(), page( 0 ) );
      }
    }
}

void KonqFrameTabs::slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */)
{
  accept = KURLDrag::canDecode( e );
}

void KonqFrameTabs::slotReceivedDropEvent( QDropEvent *e )
{
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( e, lstDragURLs );
  if ( ok && lstDragURLs.first().isValid() ) {
    KonqView* newView = m_pViewManager->addTab(QString::null, QString::null, false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openURL( newView, lstDragURLs.first(), QString::null );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotReceivedDropEvent( QWidget *w, QDropEvent *e )
{
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( e, lstDragURLs );
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
  if ( ok && lstDragURLs.first().isValid() && frame ) {
    KURL lstDragURL = lstDragURLs.first();
    if ( lstDragURL != frame->activeChildView()->url() )
      m_pViewManager->mainWindow()->openURL( frame->activeChildView(), lstDragURL );
  }
}

void KonqFrameTabs::slotInitiateDrag( QWidget *w )
{
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>( w );
  if (frame) {
    KURL::List lst;
    lst.append( frame->activeChildView()->url() );
    KURLDrag *d = new KURLDrag( lst, this );
    d->setPixmap( KMimeType::pixmapForURL( lst.first(), 0, KIcon::Small ) );
    d->dragCopy();
  }
}

#include "konq_tabs.moc"
