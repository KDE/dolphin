/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
// -*- mode: c++; c-basic-offset: 2 -*-

#include "konq_viewmgr.h"
#include "konq_view.h"
#include "konq_frame.h"
#include "konq_profiledlg.h"

#include <qfileinfo.h>
#include <qptrlist.h>

#include <kaccelgen.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kapplication.h>
#include <ktempfile.h>
#include <dcopclient.h>

#include <assert.h>
#include <kpopupmenu.h>

template class QPtrList<KonqView>;

KonqViewManager::KonqViewManager( KonqMainWindow *mainWindow )
 : KParts::PartManager( mainWindow )
{
  m_pMainWindow = mainWindow;
  m_pDocContainer = 0L;

  m_pamProfiles = 0L;
  m_bProfileListDirty = true;
  m_bLoadingProfile = false;

  connect( this, SIGNAL( activePartChanged ( KParts::Part * ) ), this, SLOT( slotActivePartChanged ( KParts::Part * ) ) );
}

KonqView* KonqViewManager::Initialize( const QString &serviceType, const QString &serviceName )
{
  kdDebug(1202) << "KonqViewManager::Initialize()" << endl;
  KService::Ptr service;
  KTrader::OfferList partServiceOffers, appServiceOffers;
  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers, true );
  KonqView* childView = setupView( m_pMainWindow, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false );
  setActivePart( childView->part() );
  m_pDocContainer = childView->frame();
  m_pDocContainer->widget()->show();
  return childView;
}

KonqViewManager::~KonqViewManager()
{
  kdDebug(1202) << "KonqViewManager::~KonqViewManager()" << endl;
  clear();
}

KonqView* KonqViewManager::splitView ( Qt::Orientation orientation,
                                       const QString &serviceType,
                                       const QString &serviceName,
                                       bool newOneFirst, bool forceAutoEmbed )
{
  kdDebug(1202) << "KonqViewManager::splitView(ServiceType)" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  KonqFrame* splitFrame = m_pMainWindow->currentView()->frame();

  KService::Ptr service;
  KTrader::OfferList partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers, forceAutoEmbed );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  KonqView *childView;

  assert( splitFrame );

  KonqFrameContainerBase* parentContainer = splitFrame->parentContainer();
  
  bool moveNewContainer = false;
  QValueList<int> splitterSizes;
  int index= -1;

  if (parentContainer->frameType()=="Container") {
    moveNewContainer = (static_cast<KonqFrameContainer*>(parentContainer)->idAfter( splitFrame->widget() ) != 0);
    splitterSizes = static_cast<KonqFrameContainer*>(parentContainer)->sizes();
  }
  else if (parentContainer->frameType()=="Tabs")
    index = static_cast<KonqFrameTabs*>(parentContainer)->indexOf( splitFrame->widget() );

#ifndef NDEBUG
  printSizeInfo( splitFrame, parentContainer, "before split");
#endif

  parentContainer->widget()->setUpdatesEnabled( false );

  //kdDebug(1202) << "Move out child" << endl;
  QPoint pos = splitFrame->widget()->pos();
  parentContainer->removeChildFrame( splitFrame );
  splitFrame->widget()->reparent( m_pMainWindow, pos );

  //kdDebug(1202) << "Create new Container" << endl;
  KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer->widget(), parentContainer );
  connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
  newContainer->setOpaqueResize( true );

  parentContainer->insertChildFrame( newContainer, index );
  if ( moveNewContainer ) {
    static_cast<KonqFrameContainer*>(parentContainer)->moveToFirst( newContainer );
    static_cast<KonqFrameContainer*>(parentContainer)->swapChildren();
  }

  //kdDebug(1202) << "Move in child" << endl;
  splitFrame->widget()->reparent( newContainer, pos );
  newContainer->insertChildFrame( splitFrame );

#ifndef NDEBUG
  printSizeInfo( splitFrame, parentContainer, "after reparent" );
#endif

  //kdDebug(1202) << "Create new child" << endl;
  childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false );

#ifndef DEBUG
  printSizeInfo( splitFrame, parentContainer, "after child insert" );
#endif

  if ( newOneFirst )
  {
    newContainer->moveToLast( splitFrame->widget() );
    newContainer->swapChildren();
  }

  QValueList<int> newSplitterSizes;
  newSplitterSizes << 50 << 50;
  newContainer->setSizes( newSplitterSizes );

  if (parentContainer->frameType()=="Container") {
    static_cast<KonqFrameContainer*>(parentContainer)->setSizes( splitterSizes );
  }
  else if (parentContainer->frameType()=="Tabs")
    static_cast<KonqFrameTabs*>(parentContainer)->showPage( newContainer );

  splitFrame->show();
  newContainer->show();

  newContainer->setActiveChild( splitFrame );
  setActivePart( splitFrame->part(), true );

  parentContainer->widget()->setUpdatesEnabled( true );

  if (m_pDocContainer == splitFrame) m_pDocContainer = newContainer;

  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kdDebug(1202) << "KonqViewManager::splitView(ServiceType) done" << endl;

  return childView;
}

KonqView* KonqViewManager::splitWindow( Qt::Orientation orientation,
                                        const QString &serviceType,
                                        const QString &serviceName,
                                        bool newOneFirst )
{
  kdDebug(1202) << "KonqViewManager::splitWindow(default)" << endl;

  KURL url = m_pMainWindow->currentView()->url();
  QString locationBarURL = m_pMainWindow->currentView()->locationBarURL();

  KService::Ptr service;
  KTrader::OfferList partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  KonqFrameBase* mainFrame = m_pMainWindow->childFrame();

  mainFrame->widget()->setUpdatesEnabled( false );

  //kdDebug(1202) << "Move out child" << endl;
  QPoint pos = mainFrame->widget()->pos();
  m_pMainWindow->removeChildFrame( mainFrame );

  KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, m_pMainWindow, 0L);
  connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
  newContainer->setOpaqueResize( true );

  m_pMainWindow->insertChildFrame( newContainer );

  newContainer->insertChildFrame( mainFrame );
  mainFrame->widget()->reparent( newContainer, pos );

  KonqView* childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, true );

  if( newOneFirst ) {
      static_cast<KonqFrameContainer*>(newContainer)->moveToFirst( childView->frame() );
      static_cast<KonqFrameContainer*>(newContainer)->swapChildren();
  }

  newContainer->show();

  mainFrame->widget()->setUpdatesEnabled( true );

  if( childView )
    childView->openURL( url, locationBarURL );

  newContainer->setActiveChild( mainFrame );

  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kdDebug(1202) << "KonqViewManager::splitWindow(default) done" << endl;

  return childView;
}

void KonqViewManager::convertDocContainer()
{
  // Must create a tab container since one is not present,
  // then insert the existing frame as a tab

  KonqFrameContainerBase* parentContainer = m_pDocContainer->parentContainer();

  bool moveNewContainer = false;
  QValueList<int> splitterSizes;
  if (parentContainer->frameType()=="Container") {
    moveNewContainer = (static_cast<KonqFrameContainer*>(parentContainer)->idAfter( m_pDocContainer->widget() ) != 0);
    splitterSizes = static_cast<KonqFrameContainer*>(parentContainer)->sizes();
  }

  parentContainer->widget()->setUpdatesEnabled( false );

  //kdDebug(1202) << "Move out child" << endl;
  QPoint pos = m_pDocContainer->widget()->pos();
  parentContainer->removeChildFrame( m_pDocContainer );
  m_pDocContainer->widget()->reparent( m_pMainWindow, pos );

  KonqFrameTabs* newContainer = new KonqFrameTabs( parentContainer->widget() , parentContainer, this);
  parentContainer->insertChildFrame( newContainer );
  connect( newContainer, SIGNAL(ctrlTabPressed()), m_pMainWindow, SLOT(slotCtrlTabPressed()) );

  m_pDocContainer->widget()->reparent( newContainer, pos );
  newContainer->insertChildFrame( m_pDocContainer );

  if ( moveNewContainer ) {
    static_cast<KonqFrameContainer*>(parentContainer)->moveToFirst( newContainer );
    static_cast<KonqFrameContainer*>(parentContainer)->swapChildren();
  }
  if (parentContainer->frameType()=="Container")
    static_cast<KonqFrameContainer*>(parentContainer)->setSizes( splitterSizes );

  newContainer->show();

  parentContainer->widget()->setUpdatesEnabled( true );

  m_pDocContainer = newContainer;
}

void KonqViewManager::revertDocContainer()
{
  // If the tab container is left with only one tab after the removal,
  // destroy it and put its lone child frame in its place

  KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(m_pDocContainer);

  KonqFrameContainerBase* parentContainer = tabContainer->parentContainer();
  kdDebug(1202) << "parentContainer=" << parentContainer << endl;
  if (parentContainer == 0L) return;

  bool moveNewContainer = false;
  QValueList<int> splitterSizes;
  if (parentContainer->frameType()=="Container") {
    moveNewContainer = (static_cast<KonqFrameContainer*>(parentContainer)->idAfter( tabContainer ) != 0);
    splitterSizes = static_cast<KonqFrameContainer*>(parentContainer)->sizes();
  }

  KonqFrameBase* otherFrame = tabContainer->childFrameList()->first();
  kdDebug(1202) << "otherFrame=" << otherFrame << endl;
  if (otherFrame == 0L ) return;

  parentContainer->widget()->setUpdatesEnabled( false );

  QPoint pos = otherFrame->widget()->pos();
  otherFrame->reparentFrame( m_pMainWindow, pos );

  tabContainer->removeChildFrame( otherFrame );
  parentContainer->removeChildFrame( tabContainer );

  delete tabContainer;

  otherFrame->reparentFrame( parentContainer->widget(), pos );
  parentContainer->insertChildFrame( otherFrame );

  if ( moveNewContainer ) {
    static_cast<KonqFrameContainer*>(parentContainer)->moveToFirst( otherFrame->widget() );
    static_cast<KonqFrameContainer*>(parentContainer)->swapChildren();
  }
  if (parentContainer->frameType()=="Container")
    static_cast<KonqFrameContainer*>(parentContainer)->setSizes( splitterSizes );

  otherFrame->widget()->show();

  parentContainer->widget()->setUpdatesEnabled( true );

  parentContainer->setActiveChild( otherFrame );

  m_pDocContainer = otherFrame;
}

KonqView* KonqViewManager::addTab(const QString &serviceType, const QString &serviceName, bool passiveMode, bool forceAutoEmbed)
{
  kdDebug(1202) << "------------- KonqViewManager::addTab starting -------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  if (m_pDocContainer == 0L)
  {
    if (m_pMainWindow &&
        m_pMainWindow->currentView() &&
        m_pMainWindow->currentView()->frame()) {
       m_pDocContainer = m_pMainWindow->currentView()->frame();
    } else {
       kdDebug(1202) << "This view profile does not support tabs." << endl;
       return 0L;
    }
  }

  KService::Ptr service;
  KTrader::OfferList partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers, forceAutoEmbed );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  if (m_pDocContainer->frameType() != "Tabs") convertDocContainer();

  KonqView* childView = setupView( static_cast<KonqFrameTabs*>(m_pDocContainer), newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode );

  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kdDebug(1202) << "------------- KonqViewManager::addTab done -------------" << endl;

  return childView;
}

void KonqViewManager::duplicateTab( KonqFrameBase* tab )
{
  kdDebug(1202) << "---------------- KonqViewManager::duplicateTab( " << tab << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  if (m_pDocContainer == 0L)
  {
    if (m_pMainWindow &&
        m_pMainWindow->currentView() &&
        m_pMainWindow->currentView()->frame()) {
       m_pDocContainer = m_pMainWindow->currentView()->frame();
    } else {
       kdDebug(1202) << "This view profile does not support tabs." << endl;
       return;
    }
  }

  if (m_pDocContainer->frameType() != "Tabs") convertDocContainer();

  KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(m_pDocContainer);

  KonqFrameBase* currentFrame;
  if ( tab == 0L )
    currentFrame = dynamic_cast<KonqFrameBase*>(tabContainer->currentPage());
  else
    currentFrame = tab;

  KTempFile tempFile;
  tempFile.setAutoDelete( true );
  KConfig config( tempFile.name() );
  config.setGroup( "View Profile" );

  QString prefix = QString::fromLatin1( currentFrame->frameType() ) + QString::number(0);
  config.writeEntry( "RootItem", prefix );
  prefix.append( '_' );
  currentFrame->saveConfig( &config, prefix, true, 0L, 0, 1);

  QString rootItem = config.readEntry( "RootItem", "empty" );

  if (rootItem.isNull() || rootItem == "empty") return;

  // This flag is used by KonqView, to distinguish manual view creation
  // from profile loading (e.g. in switchView)
  m_bLoadingProfile = true;

  loadItem( config, tabContainer, rootItem, KURL(""), true );

  m_bLoadingProfile = false;

  m_pMainWindow->enableAllActions(true);

  // This flag disables calls to viewCountChanged while creating the views,
  // so we do it once at the end :
  m_pMainWindow->viewCountChanged();

  tabContainer->setCurrentPage( tabContainer->count() - 1 );
  
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kdDebug(1202) << "------------- KonqViewManager::duplicateTab done --------------" << endl;
}

void KonqViewManager::breakOffTab( KonqFrameBase* tab )
{
  kdDebug(1202) << "---------------- KonqViewManager::breakOffTab( " << tab << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  if (m_pDocContainer == 0L) return;
  if (m_pDocContainer->frameType() != "Tabs") return;

  KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(m_pDocContainer);

  KonqFrameBase* currentFrame;
  if ( tab == 0L )
    currentFrame = dynamic_cast<KonqFrameBase*>(tabContainer->currentPage());
  else
    currentFrame = tab;

  KTempFile tempFile;
  tempFile.setAutoDelete( true );
  KConfig config( tempFile.name() );
  config.setGroup( "View Profile" );

  QString prefix = QString::fromLatin1( currentFrame->frameType() ) + QString::number(0);
  config.writeEntry( "RootItem", prefix );
  prefix.append( '_' );
  currentFrame->saveConfig( &config, prefix, true, 0L, 0, 1);

  removeTab( currentFrame );
  delete currentFrame;

  KonqMainWindow *mainWindow = new KonqMainWindow( KURL(), false );
  if (mainWindow == 0L) return;

  mainWindow->viewManager()->loadViewProfile( config, "" );

  mainWindow->viewManager()->setDocContainer( mainWindow->childFrame() );

  mainWindow->enableAllActions( true );
  
  mainWindow->activateChild();

  mainWindow->show();

  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  mainWindow->dumpViewList();
  mainWindow->viewManager()->printFullHierarchy( mainWindow );

  kdDebug(1202) << "------------- KonqViewManager::breakOffTab done --------------" << endl;
}

void KonqViewManager::removeTab( KonqFrameBase* tab )
{
  kdDebug(1202) << "---------------- KonqViewManager::removeTab( " << tab << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  if (m_pDocContainer == 0L) return;
  if (m_pDocContainer->frameType() != "Tabs") return;

  KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(m_pDocContainer);
  
  KonqFrameBase* currentFrame;
  if ( tab == 0L )
    currentFrame = dynamic_cast<KonqFrameBase*>(tabContainer->currentPage());
  else
    currentFrame = tab;

  if (currentFrame->widget() == tabContainer->currentPage()) setActivePart( 0L, true );

  tabContainer->removeChildFrame(currentFrame);

  QPtrList<KonqView> viewList;
  QPtrListIterator<KonqView> it( viewList );

  currentFrame->listViews( &viewList );

  for ( it.toFirst(); it != 0L; ++it )
  {
    m_pMainWindow->removeChildView( it.current() );
    delete it.current();
  }

  delete currentFrame;

  if (tabContainer->count() == 1) revertDocContainer();

  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kdDebug(1202) << "------------- KonqViewManager::removeTab done --------------" << endl;
}

void KonqViewManager::removeView( KonqView *view )
{
  kdDebug(1202) << "---------------- removeView --------------" << view << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  KonqFrame* frame = view->frame();
  KonqFrameContainerBase* parentContainer = frame->parentContainer();

  kdDebug(1202) << "view=" << view << " frame=" << frame << " parentContainer=" << parentContainer << endl;

  if (parentContainer->frameType()=="Container")
  {
    kdDebug(1202) << "parentContainer is a KonqFrameContainer" << endl;

    KonqFrameContainerBase* grandParentContainer = parentContainer->parentContainer();
    kdDebug(1202) << "grandParentContainer=" << grandParentContainer << endl;

    setActivePart( 0L, true );

    int index = -1;
    QValueList<int> splitterSizes;
    bool moveOtherChild = false;

    if (grandParentContainer->frameType()=="Tabs")
      index = static_cast<KonqFrameTabs*>(grandParentContainer)->indexOf( parentContainer->widget() );
    else if (grandParentContainer->frameType()=="Container")
    {
      moveOtherChild = (static_cast<KonqFrameContainer*>(grandParentContainer)->idAfter( parentContainer->widget() ) != 0);
      splitterSizes = static_cast<KonqFrameContainer*>(grandParentContainer)->sizes();
    }

    KonqFrameBase* otherFrame = static_cast<KonqFrameContainer*>(parentContainer)->otherChild( frame );
    kdDebug(1202) << "otherFrame=" << otherFrame << endl;

    if( otherFrame == 0L )
    {
    kdWarning(1202) << "KonqViewManager::removeView: This shouldn't happen!" << endl;
    return;
    }

    if (m_pDocContainer == parentContainer) m_pDocContainer = otherFrame;

    grandParentContainer->widget()->setUpdatesEnabled( false );

    //kdDebug(1202) << "--- Reparenting otherFrame to m_pMainWindow " << m_pMainWindow << endl;
    QPoint pos = otherFrame->widget()->pos();
    otherFrame->reparentFrame( m_pMainWindow, pos );

    //kdDebug(1202) << "--- Removing otherFrame from parentContainer" << endl;
    parentContainer->removeChildFrame( otherFrame );

    //kdDebug(1202) << "--- Removing parentContainer from grandParentContainer" << endl;
    grandParentContainer->removeChildFrame( parentContainer );

    //kdDebug(1202) << "--- Removing view from view list, and deleting view " << view << endl;
    m_pMainWindow->removeChildView(view);
    view->partDeleted();
    delete view;

    //kdDebug(1202) << "--- Deleting parentContainer " << parentContainer
    //              << ". Its parent is " << parentContainer->widget()->parent() << endl;
    delete parentContainer;

    //kdDebug(1202) << "--- Reparenting otherFrame to grandParentContainer" << grandParentContainer << endl;
    otherFrame->reparentFrame( grandParentContainer->widget(), pos );

    //kdDebug(1202) << "--- Inserting otherFrame into grandParentContainer" << grandParentContainer << endl;
    grandParentContainer->insertChildFrame( otherFrame, index );

    if( moveOtherChild )
    {
      static_cast<KonqFrameContainer*>(grandParentContainer)->moveToFirst( otherFrame->widget() );
      static_cast<KonqFrameContainer*>(grandParentContainer)->swapChildren();
    }

    if (grandParentContainer->frameType()=="Container")
      static_cast<KonqFrameContainer*>(grandParentContainer)->setSizes( splitterSizes );

    otherFrame->widget()->show();

    grandParentContainer->setActiveChild( otherFrame );
    grandParentContainer->activateChild();
    
    grandParentContainer->widget()->setUpdatesEnabled( true );
  }
  else if (parentContainer->frameType()=="Tabs") {
    kdDebug(1202) << "parentContainer " << parentContainer << " is a KonqFrameTabs" << endl;

    removeTab();
  }
  else if (parentContainer->frameType()=="MainWindow")
    kdDebug(1202) << "parentContainer is a KonqMainWindow.  This shouldn't be removeable, not removing." << endl;
  else
    kdDebug(1202) << "Unrecognized frame type, not removing." << endl;

  printFullHierarchy( m_pMainWindow );
  m_pMainWindow->dumpViewList();

  kdDebug(1202) << "------------- removeView done --------------" << view << endl;
}

// reimplemented from PartManager
void KonqViewManager::removePart( KParts::Part * part )
{
  kdDebug(1202) << "KonqViewManager::removePart ( " << part << " )" << endl;
  // This is called when a part auto-deletes itself (case 1), or when
  // the "delete view" above deletes, in turn, the part (case 2)

  kdDebug(1202) << "Calling KParts::PartManager::removePart " << part << endl;
  KParts::PartManager::removePart( part );

  // If we were called by PartManager::slotObjectDestroyed, then the inheritance has
  // been deleted already... Can't use inherits().

  KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(part) );
  if ( view ) // the child view still exists, so we are in case 1
  {
      kdDebug(1202) << "Found a child view" << endl;
      view->partDeleted(); // tell the child view that the part auto-deletes itself
      if (m_pMainWindow->viewCount() == 1)
      {
        kdDebug(1202) << "Deleting last view -> closing the window" << endl;
        clear();
        kdDebug(1202) << "Closing m_pMainWindow " << m_pMainWindow << endl;
        m_pMainWindow->close(); // will delete it
        return;
      }
      else // normal case
        removeView( view );
  }

  kdDebug(1202) << "KonqViewManager::removePart ( " << part << " ) done" << endl;
}

void KonqViewManager::slotPassiveModePartDeleted()
{
  // Passive mode parts aren't registered to the part manager,
  // so we have to handle suicidal ones ourselves
  KParts::ReadOnlyPart * part = const_cast<KParts::ReadOnlyPart *>( static_cast<const KParts::ReadOnlyPart *>( sender() ) );
  disconnect( part, SIGNAL( destroyed() ), this, SLOT( slotPassiveModePartDeleted() ) );
  kdDebug(1202) << "KonqViewManager::slotPassiveModePartDeleted part=" << part << endl;
  KonqView * view = m_pMainWindow->childView( part );
  kdDebug(1202) << "view=" << view << endl;
  if ( view != 0L) // the child view still exists, so the part suicided
  {
    view->partDeleted(); // tell the child view that the part deleted itself
    removeView( view );
  }
}

void KonqViewManager::viewCountChanged()
{
  bool bShowActiveViewIndicator = ( m_pMainWindow->/*activeViewsCount*/viewCount() > 1 );
  bool bShowLinkedViewIndicator = ( m_pMainWindow->viewCount() > 1 );

  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  for (  ; it != end ; ++it )
  {
      it.data()->frame()->statusbar()->showActiveViewIndicator(bShowActiveViewIndicator && !it.data()->isPassiveMode());
      it.data()->frame()->statusbar()->showLinkedViewIndicator( bShowLinkedViewIndicator && (!it.data()->isFollowActive()));
  }

}
void KonqViewManager::clear()
{
  kdDebug(1202) << "KonqViewManager::clear" << endl;
  setActivePart( 0L, true /* immediate */ );

  if (m_pMainWindow->childFrame() == 0L) return;

  QPtrList<KonqView> viewList;
  QPtrListIterator<KonqView> it( viewList );

  m_pMainWindow->listViews( &viewList );

  kdDebug(1202) << viewList.count() << " items" << endl;

  for ( it.toFirst(); it.current(); ++it ) {
    m_pMainWindow->removeChildView( it.current() );
    kdDebug(1202) << "Deleting " << it.current() << endl;
    delete it.current();
  }

  kdDebug(1202) << "deleting mainFrame " << endl;
  m_pMainWindow->removeChildFrame( m_pMainWindow->childFrame() );
  delete m_pMainWindow->childFrame();

  m_pDocContainer = 0L;
}

KonqView *KonqViewManager::chooseNextView( KonqView *view )
{
  //kdDebug(1202) << "KonqViewManager(" << this << ")::chooseNextView(" << view << ")" << endl;
  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();

  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  if ( view ) // find it in the map - can't use the key since view->part() might be 0L
      while ( it != end && it.data() != view )
          ++it;

  // the view should always be in the list
   if ( it == end ) {
     if ( view )
       kdWarning() << "View " << view << " is not in list !" << endl;
     it = mapViews.begin();
     if ( it == end )
       return 0L; // We have no view at all - this happens with RootItem=empty
   }

  KonqMainWindow::MapViews::Iterator startIt = it;

  //kdDebug(1202) << "KonqViewManager::chooseNextView: count=" << mapViews.count() << endl;
  while ( true )
  {
    //kdDebug(1202) << "*KonqViewManager::chooseNextView going next" << endl;
    if ( ++it == end ) // move to next
      it = mapViews.begin(); // rewind on end

    if ( it == startIt && view )
      break; // no next view found

    KonqView *nextView = it.data();
    if ( nextView && !nextView->isPassiveMode() )
      return nextView;
    //kdDebug(1202) << "KonqViewManager::chooseNextView nextView=" << nextView << " passive=" << nextView->isPassiveMode() << endl;
  }

  //kdDebug(1202) << "KonqViewManager::chooseNextView: returning 0L" << endl;
  return 0L; // no next view found
}

KonqViewFactory KonqViewManager::createView( const QString &serviceType,
                                          const QString &serviceName,
                                          KService::Ptr &service,
                                          KTrader::OfferList &partServiceOffers,
                                          KTrader::OfferList &appServiceOffers,
					  bool forceAutoEmbed )
{
  kdDebug(1202) << "KonqViewManager::createView" << endl;
  KonqViewFactory viewFactory;

  if( serviceType.isEmpty() && m_pMainWindow->currentView() ) {
    //clone current view
    KonqView *cv = m_pMainWindow->currentView();

    viewFactory = KonqFactory::createView( cv->serviceType(), cv->service()->desktopEntryName(),
                                           &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );
  }
  else {
    //create view with the given servicetype
    viewFactory = KonqFactory::createView( serviceType, serviceName,
                                           &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );
  }

  return viewFactory;
}

KonqView *KonqViewManager::setupView( KonqFrameContainerBase *parentContainer,
                                      KonqViewFactory &viewFactory,
                                      const KService::Ptr &service,
                                      const KTrader::OfferList &partServiceOffers,
                                      const KTrader::OfferList &appServiceOffers,
                                      const QString &serviceType,
                                      bool passiveMode )
{
  kdDebug(1202) << "KonqViewManager::setupView passiveMode=" << passiveMode << endl;

  QString sType = serviceType;

  if ( sType.isEmpty() )
    sType = m_pMainWindow->currentView()->serviceType();

  //kdDebug(1202) << "KonqViewManager::setupView creating KonqFrame with parent=" << parentContainer << endl;
  KonqFrame* newViewFrame = new KonqFrame( parentContainer->widget(), parentContainer, "KonqFrame" );
  newViewFrame->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );

  kdDebug(1202) << "Creating KonqView" << endl;
  KonqView *v = new KonqView( viewFactory, newViewFrame,
                              m_pMainWindow, service, partServiceOffers, appServiceOffers, sType, passiveMode );
  kdDebug(1202) << "KonqView created" << endl;

  QObject::connect( v, SIGNAL( sigPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ),
                    m_pMainWindow, SLOT( slotPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ) );

  m_pMainWindow->insertChildView( v );

  parentContainer->insertChildFrame( newViewFrame );

  if (parentContainer->frameType() != "Tabs") newViewFrame->show();

  // Don't register passive views to the part manager
  if ( !v->isPassiveMode() ) // note that KonqView's constructor could set this to true even if passiveMode is false
    addPart( v->part(), false );
  else
  {
    // Passive views aren't registered, but we still want to detect the suicidal ones
    connect( v->part(), SIGNAL( destroyed() ), this, SLOT( slotPassiveModePartDeleted() ) );
  }

  kdDebug(1202) << "KonqViewManager::setupView done" << endl;
  return v;
}

///////////////// Profile stuff ////////////////

void KonqViewManager::saveViewProfile( const QString & fileName, const QString & profileName, bool saveURLs, bool saveWindowSize )
{

  QString path = locateLocal( "data", QString::fromLatin1( "konqueror/profiles/" ) +
                                          fileName, KGlobal::instance() );

  if ( QFile::exists( path ) )
    QFile::remove( path );

  KSimpleConfig cfg( path );
  cfg.setGroup( "Profile" );
  if ( !profileName.isEmpty() )
      cfg.writeEntry( "Name", profileName );

  saveViewProfile( cfg, saveURLs, saveWindowSize );

}

void KonqViewManager::saveViewProfile( KConfig & cfg, bool saveURLs, bool saveWindowSize )
{
  kdDebug(1202) << "KonqViewManager::saveViewProfile" << endl;
  if( m_pMainWindow->childFrame() != 0L ) {
    QString prefix = QString::fromLatin1( m_pMainWindow->childFrame()->frameType() )
                     + QString::number(0);
    cfg.writeEntry( "RootItem", prefix );
    prefix.append( '_' );
    m_pMainWindow->saveConfig( &cfg, prefix, saveURLs, m_pDocContainer, 0, 1);
  }

  if ( saveWindowSize )
  {
    cfg.writeEntry( "Width", m_pMainWindow->width() );
    cfg.writeEntry( "Height", m_pMainWindow->height() );
  }

  // Save menu/toolbar settings in profile. Relys on konq_mainwindow calling
  // setAutoSaveSetting( "KongMainWindow", false ). The false is important,
  // we do not want this call save size settings in the profile, because we
  // do it ourselves. Save in a separate group than the rest of the profile.
  QString savedGroup = cfg.group();
  m_pMainWindow->saveMainWindowSettings( &cfg, "Main Window Settings" );
  cfg.setGroup( savedGroup );

  cfg.sync();
}

void KonqViewManager::loadViewProfile( const QString & path, const QString & filename,
                                       const KURL & forcedURL, const KonqOpenURLRequest &req )
{
  KConfig cfg( path, true );
  cfg.setDollarExpansion( true );
  cfg.setGroup( "Profile" );
  loadViewProfile( cfg, filename, forcedURL, req );
}

void KonqViewManager::loadViewProfile( KConfig &cfg, const QString & filename,
                                       const KURL & forcedURL, const KonqOpenURLRequest &req )
{
  m_currentProfile = filename;
  m_currentProfileText = cfg.readEntry("Name",filename);
  m_pMainWindow->currentProfileChanged();
  KURL defaultURL;
  if ( m_pMainWindow->currentView() )
    defaultURL = m_pMainWindow->currentView()->url();

  clear();

  QString rootItem = cfg.readEntry( "RootItem", "empty" );

  kdDebug(1202) << "******************" << rootItem << endl;

  if( rootItem.isNull() ) {
    // Config file doesn't contain anything about view profiles, fallback to defaults
    rootItem = "InitialView";
  }
  kdDebug(1202) << "KonqViewManager::loadViewProfile : loading RootItem " << rootItem << endl;

  if ( rootItem != "empty" && forcedURL.url() != "about:blank" )
  {
    // This flag is used by KonqView, to distinguish manual view creation
    // from profile loading (e.g. in switchView)
    m_bLoadingProfile = true;

    loadItem( cfg, m_pMainWindow, rootItem, defaultURL, forcedURL.isEmpty() );

    m_bLoadingProfile = false;

    m_pMainWindow->enableAllActions(true);

    // This flag disables calls to viewCountChanged while creating the views,
    // so we do it once at the end :
    m_pMainWindow->viewCountChanged();
  }
  else
  {
    m_pMainWindow->disableActionsNoView();
    m_pMainWindow->action( "clear_location" )->activate();
  }

  // Set an active part first so that we open the URL in the current view
  // (to set the location bar correctly and asap)
  KonqView *nextChildView = 0L;
  nextChildView = m_pMainWindow->activeChildView();
  if (nextChildView == 0L) nextChildView = chooseNextView( 0L );
  setActivePart( nextChildView ? nextChildView->part() : 0L, true /* immediate */);

  if ( !forcedURL.isEmpty())
  {
      KonqOpenURLRequest _req(req);
      m_pMainWindow->openURL( nextChildView /* can be 0 for an empty profile */,
                              forcedURL, _req.args.serviceType, _req, _req.args.trustedSource );

      // TODO choose a linked view if any (instead of just the first one),
      // then open the same URL in any non-linked one
  }
  else
  {
      if ( m_pMainWindow->locationBarURL().isEmpty() ) // No URL -> the user will want to type one
          m_pMainWindow->focusLocationBar();
  }

  // Window size
 if ( !m_pMainWindow->initialGeometrySet() )
 {
     QSize size = readConfigSize( cfg, m_pMainWindow );
     if ( size.isValid() )
         m_pMainWindow->resize( size );
 }

  // Apply menu/toolbar settings saved in profile. Read from a separate group
  // so that the window doesn't try to change the size stored in the Profile group.
  // (If applyMainWindowSettings finds a "Width" or "Height" entry, it
  // sets them to 0,0)
  if( cfg.hasGroup( "Main Window Settings" ) ) {
    QString savedGroup = cfg.group();
    m_pMainWindow->applyMainWindowSettings( &cfg, "Main Window Settings" );
    cfg.setGroup( savedGroup );
  }

#ifndef NDEBUG
  printFullHierarchy( m_pMainWindow );
#endif

  //kdDebug(1202) << "KonqViewManager::loadViewProfile done" << endl;
}

void KonqViewManager::setActivePart( KParts::Part *part, bool immediate )
{
    kdDebug(1202) << "KonqViewManager::setActivePart " << part << endl;

    if (part == activePart())
    {
      kdDebug(1202) << "WARNING: Part is already active!" << endl;
      return;
    }

    if (part && part->widget())
        part->widget()->setFocus();

    KParts::PartManager::setActivePart( part );

    if (!immediate)
        // We use a 0s single shot timer so that when clicking on a part,
        // we process the mouse event before rebuilding the GUI.
        // Otherwise, when e.g. dragging icons, the mouse pointer can already
        // be very far from where it was...
        QTimer::singleShot( 0, this, SLOT( emitActivePartChanged() ) );
    else
        emitActivePartChanged();
}

void KonqViewManager::slotActivePartChanged ( KParts::Part *newPart )
{
    kdDebug(1202) << "KonqViewManager::slotActivePartChanged " << newPart << endl;
    if (newPart == 0L) {
      kdDebug(1202) << "newPart = 0L , returning" << endl;
      return;
    }
    KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(newPart) );
    if (view == 0L) {
      kdDebug(1202) << "No view associated with this part" << endl;
      return;
    }
    if (view->frame()->parentContainer() == 0L) return;
    if (!m_bLoadingProfile) view->frame()->parentContainer()->setActiveChild( view->frame() );
    kdDebug(1202) << "KonqViewManager::slotActivePartChanged done" << endl;
}

void KonqViewManager::setActivePart( KParts::Part *part, QWidget * )
{
    setActivePart( part, false );
}

void KonqViewManager::emitActivePartChanged()
{
    m_pMainWindow->slotPartActivated( activePart() );
}


QSize KonqViewManager::readConfigSize( KConfig &cfg, QWidget *widget )
{
    bool ok;

    QString widthStr = cfg.readEntry( "Width" );
    QString heightStr = cfg.readEntry( "Height" );

    int width = -1;
    int height = -1;

    if ( widthStr.contains( '%' ) == 1 )
    {
        widthStr.truncate( widthStr.length() - 1 );
        int relativeWidth = widthStr.toInt( &ok );
        if ( ok ) {
            int screen = widget ? QApplication::desktop()->screenNumber(widget)
                                : -1;
            width = relativeWidth * QApplication::desktop()->screenGeometry(screen).width() / 100;
        }
    }
    else
    {
        width = widthStr.toInt( &ok );
        if ( !ok )
            width = -1;
    }

    if ( heightStr.contains( '%' ) == 1 )
    {
        heightStr.truncate( heightStr.length() - 1 );
        int relativeHeight = heightStr.toInt( &ok );
        if ( ok ) {
            int screen = widget ? QApplication::desktop()->screenNumber(widget)
                                : -1;
            height = relativeHeight * QApplication::desktop()->screenGeometry(screen).height() / 100;
        }
    }
    else
    {
        height = heightStr.toInt( &ok );
        if ( !ok )
            height = -1;
    }

    return QSize( width, height );
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainerBase *parent,
                                const QString &name, const KURL & defaultURL, bool openURL )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + '_';

  kdDebug(1202) << "begin loadItem: " << name << endl;

  if( name.startsWith("View") ) {
    kdDebug(1202) << "Item is View" << endl;
    //load view config
    QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), "inode/directory");
    kdDebug(1202) << "ServiceType: " << serviceType << endl;

    QString serviceName = cfg.readEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ) );

    KService::Ptr service;
    KTrader::OfferList partServiceOffers, appServiceOffers;

    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers );
    if ( viewFactory.isNull() )
    {
      kdWarning(1202) << "Profile Loading Error: View creation failed" << endl;
      return; //ugh..
    }

    bool passiveMode = cfg.readBoolEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), false );

    //kdDebug(1202) << "Creating View Stuff" << endl;
    KonqView *childView = setupView( parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode );

    childView->setLinkedView( cfg.readBoolEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), false ) );
    childView->setToggleView( cfg.readBoolEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), false ) );
    if( !cfg.readBoolEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), true ) )
      childView->frame()->statusbar()->hide();

    if (cfg.readBoolEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), false ))
      m_pDocContainer = childView->frame();

    if (!m_pDocContainer)
    {
      if (parent->frameType() == "MainWindow")
        m_pDocContainer = childView->frame(); // Child view of mainWindow

      else if (parent->frameType() == "Container")
      {
        KonqFrameContainer* parentContainer = static_cast<KonqFrameContainer*>(parent);
        KonqFrameBase* otherFrame = parentContainer->otherChild( childView->frame() );
        if (otherFrame)
        {
          if (childView->isPassiveMode())
          {
            if (otherFrame->frameType() == "View")
            {
              KonqFrame* viewFrame = static_cast<KonqFrame*>(otherFrame);
              if (viewFrame->childView()->isPassiveMode())
                m_pDocContainer = parentContainer; // Both views are passive, shouldn't happen
              else
                m_pDocContainer = viewFrame; // This one is passive, the other is active
            }
          }
          else
          {
            if (otherFrame->frameType() == "View")
            {
              KonqFrame* viewFrame = static_cast<KonqFrame*>(otherFrame);
              if (viewFrame->childView()->isPassiveMode())
                m_pDocContainer = childView->frame(); // This one is active, the other is passive
              else
                m_pDocContainer = parentContainer; // Both views are active
            }
            else
              m_pDocContainer = parentContainer; // This one is active, the other is a Container
          }
        }
      }
    }

    childView->frame()->show();

    QString key = QString::fromLatin1( "URL" ).prepend( prefix );
    if ( openURL )
    {
      KURL url( defaultURL );
      if ( cfg.hasKey( key ) ) // if it has it, we load it, even if empty
        url = KURL( cfg.readEntry( key ) );

      if ( !url.isEmpty() )
      {
        //kdDebug(1202) << "loadItem: calling openURL " << url.prettyURL() << endl;
        //childView->openURL( url, url.prettyURL() );
        // We need view-follows-view (for the dirtree, for instance)
        KonqOpenURLRequest req;
        if (url.protocol() != "about")
          req.typedURL = url.prettyURL();
        m_pMainWindow->openView( serviceType, url, childView, req );
      }
    }
    // Do this after opening the URL, so that it's actually possible to open it :)
    childView->setLockedLocation( cfg.readBoolEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), false ) );
  }
  else if( name.startsWith("Container") ) {
    kdDebug(1202) << "Item is Container" << endl;

    //load container config
    QString ostr = cfg.readEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ) );
    kdDebug(1202) << "Orientation: " << ostr << endl;
    Qt::Orientation o;
    if( ostr == "Vertical" )
      o = Qt::Vertical;
    else if( ostr == "Horizontal" )
      o = Qt::Horizontal;
    else {
        kdWarning() << "Profile Loading Error: No orientation specified in " << name << endl;
      o = Qt::Horizontal;
    }

    QValueList<int> sizes =
        cfg.readIntListEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ));

    int index = cfg.readNumEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), -1 );

    QStrList childList;
    if( cfg.readListEntry( QString::fromLatin1( "Children" ).prepend( prefix ), childList ) < 2 )
    {
      kdWarning() << "Profile Loading Error: Less than two children in " << name << endl;
      // fallback to defaults
      loadItem( cfg, parent, "InitialView", defaultURL, openURL );
    }
    else
    {
      KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent->widget(), parent );
      connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
      parent->insertChildFrame( newContainer );

      if (cfg.readBoolEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), false ))
        m_pDocContainer = newContainer;

      loadItem( cfg, newContainer, childList.at(0), defaultURL, openURL );
      loadItem( cfg, newContainer, childList.at(1), defaultURL, openURL );

      newContainer->setSizes( sizes );

      if (index == 1)
        newContainer->setActiveChild( newContainer->secondChild() );
      else if (index == 0)
        newContainer->setActiveChild( newContainer->firstChild() );

      newContainer->show();
    }
  }
  else if( name.startsWith("Tabs") )
  {
    kdDebug(1202) << "Item is a Tabs" << endl;

    KonqFrameTabs *newContainer = new KonqFrameTabs( parent->widget(), parent, this );
    connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
    parent->insertChildFrame( newContainer );
    m_pDocContainer = newContainer;

    int index = cfg.readNumEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), 0 );

    QStringList childList = cfg.readListEntry( QString::fromLatin1( "Children" ).prepend( prefix ) );
    for ( QStringList::Iterator it = childList.begin(); it != childList.end(); ++it )
    {
        loadItem( cfg, newContainer, *it, defaultURL, openURL );
        KonqView* activeChildView = dynamic_cast<KonqFrameBase*>(newContainer->currentPage())->activeChildView();
        if (activeChildView != 0L) {
          activeChildView->setCaption( activeChildView->caption() );
          activeChildView->setTabIcon( activeChildView->url().url() );
        }
    }

    newContainer->setActiveChild( dynamic_cast<KonqFrameBase*>(newContainer->page(index)) );
    newContainer->setCurrentPage( index );

    newContainer->show();
  }
  else
      kdWarning() << "Profile Loading Error: Unknown item " << name;

  kdDebug(1202) << "end loadItem: " << name << endl;
}

void KonqViewManager::setProfiles( KActionMenu *profiles )
{
  m_pamProfiles = profiles;

  if ( m_pamProfiles )
  {
    connect( m_pamProfiles->popupMenu(), SIGNAL( activated( int ) ),
             this, SLOT( slotProfileActivated( int ) ) );
    connect( m_pamProfiles->popupMenu(), SIGNAL( aboutToShow() ),
             this, SLOT( slotProfileListAboutToShow() ) );
  }
  //KonqMainWindow::enableAllActions will call it anyway
  //profileListDirty();
}

void KonqViewManager::showProfileDlg( const QString & preselectProfile )
{
  KonqProfileDlg dlg( this, preselectProfile, m_pMainWindow );
  dlg.exec();
  profileListDirty();
}

void KonqViewManager::slotProfileDlg()
{
  showProfileDlg( QString::null );
}

void KonqViewManager::profileListDirty( bool broadcast )
{
  kdDebug(1202) << "KonqViewManager::profileListDirty()" << endl;
  if ( !broadcast )
  {
    m_bProfileListDirty = true;
#if 0
  // There's always one profile at least, now...
  QStringList profiles = KonqFactory::instance()->dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
  if ( m_pamProfiles )
      m_pamProfiles->setEnabled( profiles.count() > 0 );
#endif
    return;
  }

  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "updateProfileList()", QByteArray() );
}

void KonqViewManager::slotProfileActivated( int id )
{

  QMap<QString, QString>::ConstIterator iter = m_mapProfileNames.begin();
  QMap<QString, QString>::ConstIterator end = m_mapProfileNames.end();

  for(int i=0; iter != end; ++iter, ++i) {
    if( i == id ) {
      KURL u;
      u.setPath( *iter );
      loadViewProfile( *iter, u.fileName() );
      break;
    }
  }

}

void KonqViewManager::slotProfileListAboutToShow()
{
  if ( !m_pamProfiles || !m_bProfileListDirty )
    return;

  QPopupMenu *popup = m_pamProfiles->popupMenu();
  popup->clear();

  // Fetch profiles
  m_mapProfileNames = KonqProfileDlg::readAllProfiles();

  // Generate accelerators
  QStringList accel_strings;
  KAccelGen::generateFromKeys(m_mapProfileNames, accel_strings);

  // Store menu items
  QValueListIterator<QString> iter = accel_strings.begin();
  for( int id = 0;
       iter != accel_strings.end();
       ++iter, ++id ) {
      popup->insertItem( *iter, id );
  }

  m_bProfileListDirty = false;
}

///////////////// Debug stuff ////////////////

void KonqViewManager::printSizeInfo( KonqFrameBase* frame,
                                     KonqFrameContainerBase* parent,
                                     const char* msg )
{
  QRect r;
  r = frame->widget()->geometry();
  qDebug("Child size %s : x: %d, y: %d, w: %d, h: %d", msg, r.x(),r.y(),r.width(),r.height() );

  if ( parent->frameType() == "Container" )
  {
  QValueList<int> sizes;
    sizes = static_cast<KonqFrameContainer*>(parent)->sizes();
  printf( "Parent sizes %s :", msg );
  QValueList<int>::ConstIterator it;
  for( it = sizes.begin(); it != sizes.end(); ++it )
    printf( " %d", (*it) );
  printf("\n");
  }
}

void KonqViewManager::printFullHierarchy( KonqFrameContainerBase * container )
{
  kdDebug(1202) << "currentView=" << m_pMainWindow->currentView() << endl;
  kdDebug(1202) << "docContainer=" << m_pDocContainer << endl;

  if (container) container->printFrameInfo(QString::null);
  else m_pMainWindow->printFrameInfo(QString::null);
}

#include "konq_viewmgr.moc"
