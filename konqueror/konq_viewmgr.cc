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

#include <kparts/browserextension.h>
#include "konq_viewmgr.h"
#include "konq_mainwindow.h"
#include "konq_view.h"
#include "konq_factory.h"
#include "konq_frame.h"
#include "konq_profiledlg.h"

#include <qstringlist.h>
#include <qdir.h>
#include <qevent.h>
#include <qapplication.h>
#include <qfileinfo.h>
#include <qapplication.h>

#include <kaccelgen.h>
#include <kaction.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kpopupmenu.h>
#include <kapp.h>
#include <dcopclient.h>

#include <assert.h>

template class QList<KonqView>;

KonqViewManager::KonqViewManager( KonqMainWindow *mainWindow )
 : KParts::PartManager( mainWindow )
{
  m_pMainWindow = mainWindow;

  m_pMainContainer = 0L;

  m_pamProfiles = 0L;
  m_bProfileListDirty = true;
  m_bLoadingProfile = false;
}

KonqViewManager::~KonqViewManager()
{
  clear();
}

KonqView* KonqViewManager::splitView ( Qt::Orientation orientation,
                                       const QString &serviceType,
                                       const QString &serviceName,
                                       bool newOneFirst)
{
  kdDebug(1202) << "KonqViewManager::splitView(ServiceType)" << endl;

  KonqFrame* viewFrame = 0L;
  if( m_pMainContainer )
  {
    if ( m_pMainWindow->currentView() )
      viewFrame = m_pMainWindow->currentView()->frame();
    else
    {
      kdWarning(1202) << "splitView called, but no current view!" << endl;
      return 0L; // if we go on, we'll hit the assert in split()
    }
  }

  KonqFrameContainer *newContainer;
  KonqView* childView = split( viewFrame, orientation, serviceType, serviceName, &newContainer );

  if ( newOneFirst )
  {
      newContainer->moveToLast( viewFrame->widget() );
      newContainer->swapChildren();
  }

  return childView;
}

KonqView* KonqViewManager::splitWindow( Qt::Orientation orientation,
                                        const QString &serviceType,
                                        const QString &serviceName,
                                        bool newOneFirst )
{
  kdDebug(1202) << "KonqViewManager::splitWindow(default)" << endl;

  KURL url = m_pMainWindow->currentView()->url();

  QString locationBarURL;
  KonqFrameBase* splitFrame = 0L;
  if( m_pMainContainer )
  {
    splitFrame = m_pMainContainer->firstChild();
    locationBarURL = m_pMainWindow->currentView()->locationBarURL();
    if ( !splitFrame )
    {
      kdWarning(1202) << "splitFrame called, but no view in m_pMainContainer!" << endl;
      return 0L; // if we go on, we'll hit the assert in split()
    }
  }

  KonqFrameContainer *newContainer;
  KonqView* childView = split( splitFrame, orientation, serviceType, serviceName, &newContainer );

  if ( newOneFirst )
  {
      newContainer->moveToLast( splitFrame->widget() );
      newContainer->swapChildren();
  }

  if( childView )
    childView->openURL( url, locationBarURL );

  return childView;
}

KonqView* KonqViewManager::split (KonqFrameBase* splitFrame,
                                  Qt::Orientation orientation,
                                  const QString &serviceType, const QString &serviceName,
                                  KonqFrameContainer **newFrameContainer,
                                  bool passiveMode )
{
  kdDebug(1202) << "KonqViewManager::split" << endl;

  KService::Ptr service;
  KTrader::OfferList partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  KonqView *childView;
  if( m_pMainContainer )
  {
    assert( splitFrame );

    KonqFrameContainer* parentContainer = splitFrame->parentContainer();
    bool moveNewContainer = (parentContainer->idAfter( splitFrame->widget() ) != 0);

#ifndef NDEBUG
    printSizeInfo( splitFrame, parentContainer, "before split");
#endif

    splitFrame->widget()->setUpdatesEnabled( false );
    parentContainer->setUpdatesEnabled( false );

    QPoint pos = splitFrame->widget()->pos();

    //kdDebug(1202) << "Move out child" << endl;
    parentContainer->removeChildFrame( splitFrame );
    splitFrame->widget()->reparent( m_pMainWindow, pos );
    splitFrame->widget()->hide(); // Can't hide before, but after is better than nothing
    splitFrame->widget()->resize( 100, 30 ); // bring it to the QWidget defaultsize, so that both container childs are equally size after split

    //kdDebug(1202) << "Create new Container" << endl;
    KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer );
    connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
    newContainer->setUpdatesEnabled( false );
    newContainer->setOpaqueResize( true );
    newContainer->show();

    parentContainer->insertChildFrame( newContainer );
    if( moveNewContainer )
      parentContainer->moveToFirst( newContainer );

    //kdDebug(1202) << "Move in child" << endl;
    splitFrame->widget()->reparent( newContainer, pos, true /*showIt*/ );
    newContainer->insertChildFrame( splitFrame );

#ifndef NDEBUG
    printSizeInfo( splitFrame, parentContainer, "after reparent" );
#endif

    //kdDebug(1202) << "Create new child" << endl;
    childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode );

#ifndef DEBUG
    printSizeInfo( splitFrame, parentContainer, "after child insert" );
#endif

    splitFrame->widget()->setUpdatesEnabled( true );
    newContainer->setUpdatesEnabled( true );
    parentContainer->setUpdatesEnabled( true );

    if ( newFrameContainer )
      *newFrameContainer = newContainer;

  }
  else // We had no main container, create one
  {
    m_pMainContainer = new KonqFrameContainer( orientation, m_pMainWindow );
    kdDebug(1202) << "Created main container " << m_pMainContainer << endl;
    connect(m_pMainContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
    m_pMainWindow->setCentralWidget( m_pMainContainer );
    m_pMainContainer->setOpaqueResize();
    m_pMainContainer->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );

    childView = setupView( m_pMainContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode );

    //m_pMainContainer->show();

    //childView->frame()->statusbar()->hideStuff();

    if ( newFrameContainer )
      *newFrameContainer = m_pMainContainer;
  }

#ifndef NDEBUG
  printFullHierarchy( m_pMainContainer );
#endif

  return childView;
}

void KonqViewManager::removeView( KonqView *view )
{
  kdDebug(1202) << "---------------- removeView " << view << endl;
  if ( activePart() == view->part() )
  {
    KonqView *nextView = chooseNextView( view );
    // Don't remove the last view
    if ( nextView == 0L )
      return;

    // Ensure this is not the active view anymore
    kdDebug(1202) << "Setting part " << nextView->part() << " as active" << endl;
    if ( nextView->part() )
      setActivePart( nextView->part(), true /*immediate */ );
  }

  KonqFrameContainer* parentContainer = view->frame()->parentContainer();
  KonqFrameContainer* grandParentContainer = parentContainer->parentContainer();
  kdDebug(1202) << "view=" << view << " parentContainer=" << parentContainer
                << " grandParentContainer=" << grandParentContainer << endl;
  bool moveOtherChild = (grandParentContainer->idAfter( parentContainer ) != 0);

  KonqFrameBase* otherFrame = parentContainer->otherChild( view->frame() );

  if( otherFrame == 0L ) {
    kdWarning(1202) << "KonqViewManager::removeView: This shouldn't happen!" << endl;
    return;
  }

  kdDebug(1202) << "KonqViewManager::removeView reparenting other frame " << otherFrame << " widget=" << otherFrame->widget() << endl;
  QPoint pos = otherFrame->widget()->pos();

  otherFrame->reparentFrame( m_pMainWindow, pos );
  otherFrame->widget()->hide(); // Can't hide before, but after is better than nothing
  otherFrame->widget()->resize( 100, 30 ); // bring it to the QWidget defaultsize
  parentContainer->removeChildFrame( otherFrame );

  m_pMainWindow->removeChildView( view );

  parentContainer->removeChildFrame( view->frame() );

  // We did so ourselves for passive views - doesn't really matter, but saves a call to slotPassiveModePartDeleted
  if ( view->isPassiveMode() && view->part() )
      disconnect( view->part(), SIGNAL( destroyed() ), this, SLOT( slotPassiveModePartDeleted() ) );

  kdDebug(1202) << "Deleting view frame " << view->frame() << endl;
  delete view->frame();
  // This deletes the widgets inside, including the part's widget, so tell the child view
  view->partDeleted();
  kdDebug(1202) << "Deleting view " << view << endl;
  delete view;

  grandParentContainer->removeChildFrame( parentContainer );
  kdDebug(1202) << "Deleting parentContainer " << parentContainer
                << ". Its parent is " << parentContainer->parent() << endl;
  delete parentContainer;

  otherFrame->reparentFrame( grandParentContainer, pos, true/*showIt*/ );
  grandParentContainer->insertChildFrame( otherFrame );
  if( moveOtherChild )
    grandParentContainer->moveToFirst( otherFrame->widget() );

  kdDebug(1202) << "------------- removeView done " << view << endl;
#ifndef NDEBUG
  printFullHierarchy( m_pMainContainer );
#endif
}

// reimplemented from PartManager
void KonqViewManager::removePart( KParts::Part * part )
{
  kdDebug(1202) << "KonqViewManager::removePart ( " << part << " )" << endl;
  // This is called when a part auto-deletes itself (case 1), or when
  // the "delete view" above deletes, in turn, the part (case 2)

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
        setActivePart( 0L, true );
        m_pMainWindow->removeChildView( view );
        kdDebug(1202) << "Deleting frame " << view << endl;
        delete view->frame(); // This deletes the widgets inside, including the part's widget
        kdDebug(1202) << "Deleting view " << view << endl;
        delete view;
        kdDebug(1202) << "Deleting m_pMainContainer " << m_pMainContainer << endl;
        delete m_pMainContainer;
        m_pMainContainer = 0L;
        kdDebug(1202) << "Closing m_pMainWindow " << m_pMainWindow << endl;
        m_pMainWindow->close(); // will delete it
        return;
      }
      else // normal case
        removeView( view );
  }

  //kdDebug(1202) << "Calling KParts::PartManager::removePart " << part << endl;
  KParts::PartManager::removePart( part );
  //kdDebug(1202) << "KonqViewManager::removePart ( " << part << " ) done" << endl;
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
  if ( view ) // the child view still exists, so the part suicided
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
      it.data()->frame()->statusbar()->showActiveViewIndicator(
          bShowActiveViewIndicator && !it.data()->isPassiveMode()
      );
      it.data()->frame()->statusbar()->showLinkedViewIndicator( bShowLinkedViewIndicator );
  }

}
void KonqViewManager::clear()
{
  kdDebug(1202) << "KonqViewManager::clear" << endl;
  setActivePart( 0L, true /* immediate */ );
  QList<KonqView> viewList;
  QListIterator<KonqView> it( viewList );

  if (m_pMainContainer) {
    m_pMainContainer->listViews( &viewList );

    for ( it.toFirst(); it.current(); ++it ) {
      m_pMainWindow->removeChildView( it.current() );
      delete it.current();
    }

    kdDebug(1202) << "deleting m_pMainContainer " << endl;
    delete m_pMainContainer;
    m_pMainContainer = 0L;
  }
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
                                          KTrader::OfferList &appServiceOffers )
{
  kdDebug(1202) << "KonqViewManager::createView" << endl;
  KonqViewFactory viewFactory;

  if( serviceType.isEmpty() && m_pMainWindow->currentView() ) {
    //clone current view
    KonqView *cv = m_pMainWindow->currentView();

    viewFactory = KonqFactory::createView( cv->serviceType(), cv->service()->desktopEntryName(),
                                           &service, &partServiceOffers, &appServiceOffers );
  }
  else {
    //create view with the given servicetype
    viewFactory = KonqFactory::createView( serviceType, serviceName,
                                           &service, &partServiceOffers, &appServiceOffers );
  }

  return viewFactory;
}

KonqView *KonqViewManager::setupView( KonqFrameContainer *parentContainer,
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
  KonqFrame* newViewFrame = new KonqFrame( parentContainer, "KonqFrame" );
  parentContainer->insertChildFrame( newViewFrame );

  kdDebug(1202) << "Creating KonqView" << endl;
  KonqView *v = new KonqView( viewFactory, newViewFrame,
                              m_pMainWindow, service, partServiceOffers, appServiceOffers, sType, passiveMode );
  kdDebug(1202) << "KonqView created" << endl;

  QObject::connect( v, SIGNAL( sigPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ),
                    m_pMainWindow, SLOT( slotPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ) );

  m_pMainWindow->insertChildView( v );

  newViewFrame->show();

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
  if( m_pMainContainer && m_pMainContainer->firstChild() ) {
    cfg.writeEntry( "RootItem", QString::fromLatin1( m_pMainContainer->firstChild()->frameType() )
                    + QString::number(0) );
    QString prefix = QString::fromLatin1( m_pMainContainer->firstChild()->frameType() )
                     + QString::number(0);
    prefix.append( '_' );
    m_pMainContainer->firstChild()->saveConfig( &cfg, prefix, saveURLs, 0, 1 );
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

  if( rootItem.isNull() ) {
    // Config file doesn't contain anything about view profiles, fallback to defaults
    rootItem = "InitialView";
  }
  kdDebug(1202) << "KonqViewManager::loadViewProfile : loading RootItem " << rootItem << endl;

  if ( rootItem != "empty" && forcedURL.url() != "about:blank" )
  {
      m_pMainContainer = new KonqFrameContainer( Qt::Horizontal, m_pMainWindow );
      connect(m_pMainContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
      m_pMainWindow->setCentralWidget( m_pMainContainer );
      m_pMainContainer->setOpaqueResize();
      m_pMainContainer->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );
      m_pMainContainer->show();

      // This flag is used by KonqView, to distinguish manual view creation
      // from profile loading (e.g. in switchView)
      m_bLoadingProfile = true;

      loadItem( cfg, m_pMainContainer, rootItem, defaultURL, forcedURL.isEmpty() );

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
  KonqView *nextChildView = chooseNextView( 0L );
  setActivePart( nextChildView ? nextChildView->part() : 0L, true /* immediate */ );

  if ( !forcedURL.isEmpty())
  {
      KonqOpenURLRequest _req(req);
      if (_req.typedURL.isEmpty())
          _req.typedURL = forcedURL.url();
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
     QSize size = readConfigSize( cfg );
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
  printFullHierarchy( m_pMainContainer );
#endif

  //kdDebug(1202) << "KonqViewManager::loadViewProfile done" << endl;
}

void KonqViewManager::setActivePart( KParts::Part *part, bool immediate )
{
    kdDebug(1202) << "KonqViewManager::setActivePart " << part << endl;
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

void KonqViewManager::setActivePart( KParts::Part *part, QWidget * )
{
    setActivePart( part, false );
}

void KonqViewManager::emitActivePartChanged()
{
    m_pMainWindow->slotPartActivated( activePart() );
}

QSize KonqViewManager::readConfigSize( KConfig &cfg )
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
        if ( ok )
            width = relativeWidth * QApplication::desktop()->width() / 100;
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
        if ( ok )
            height = relativeHeight * QApplication::desktop()->height() / 100;
    }
    else
    {
        height = heightStr.toInt( &ok );
        if ( !ok )
            height = -1;
    }

    return QSize( width, height );
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainer *parent,
                                const QString &name, const KURL & defaultURL, bool openURL )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + '_';

  kdDebug(1202) << "begin loadItem: " << name << endl;

  if( name.find("View") != -1 ) {
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
  else if( name.find("Container") != -1 ) {
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

    QStrList childList;
    if( cfg.readListEntry( QString::fromLatin1( "Children" ).prepend( prefix ), childList ) < 2 )
    {
      kdWarning() << "Profile Loading Error: Less than two children in " << name << endl;
      // fallback to defaults
      loadItem( cfg, parent, "InitialView", defaultURL, openURL );
    }
    else
    {
      KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent );
      connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));
      parent->insertChildFrame( newContainer );
      newContainer->setOpaqueResize();
      newContainer->show();

      loadItem( cfg, newContainer, childList.at(0), defaultURL, openURL );
      loadItem( cfg, newContainer, childList.at(1), defaultURL, openURL );

      newContainer->setSizes( sizes );
    }
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
                                     KonqFrameContainer* parent,
                                     const char* msg )
{
  QRect r;
  r = frame->widget()->geometry();
  qDebug("Child size %s : x: %d, y: %d, w: %d, h: %d", msg, r.x(),r.y(),r.width(),r.height() );

  QValueList<int> sizes;
  sizes = parent->sizes();
  printf( "Parent sizes %s :", msg );
  QValueList<int>::ConstIterator it;
  for( it = sizes.begin(); it != sizes.end(); ++it )
    printf( " %d", (*it) );
  printf("\n");
}

void KonqViewManager::printFullHierarchy( KonqFrameContainer * container, int ident )
{
    if (container)
    {
        QString spaces;
        for ( int i = 0 ; i < ident ; i++ )
            spaces += " ";
        kdDebug(1202) << spaces << "Container " << container << endl;
        KonqFrameBase * child = container->firstChild();
        if ( !child )
            kdDebug(1202) << spaces << "  Null child" << endl;
        else if ( child->widget()->isA("KonqFrameContainer") )
            printFullHierarchy( static_cast<KonqFrameContainer *>(child), ident + 2 );
        else
            kdDebug(1202) << spaces << "  " << "KonqFrame containing view "
                          << static_cast<KonqFrame *>(child)->childView()
                          << " part "
                          << static_cast<KonqFrame *>(child)->part()
                          << " whose widget is a "
                          << static_cast<KonqFrame *>(child)->part()->widget()->className() << endl;
        child = container->secondChild();
        if ( !child )
            kdDebug(1202) << spaces << "  Null child" << endl;
        else if ( child->widget()->isA("KonqFrameContainer") )
            printFullHierarchy( static_cast<KonqFrameContainer *>(child), ident + 2 );
        else
            kdDebug(1202) << spaces << "  " << "KonqFrame " << child << " containing view "
                          << static_cast<KonqFrame *>(child)->childView()
                          << " part "
                          << static_cast<KonqFrame *>(child)->part()
                          << " whose widget is a "
                          << static_cast<KonqFrame *>(child)->part()->widget()->className() << endl;
    }
    else
        kdDebug(1202) << "Null container" << endl;
}

#include "konq_viewmgr.moc"
