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

#include <kaction.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kpopupmenu.h>

#include <assert.h>

template class QList<KonqView>;

KonqViewManager::KonqViewManager( KonqMainWindow *mainWindow )
 : KParts::PartManager( mainWindow )
{
  m_pMainWindow = mainWindow;

  m_pMainContainer = 0L;

  m_pamProfiles = 0L;
  m_bProfileListDirty = true;
}

KonqViewManager::~KonqViewManager()
{
  clear();
}

KonqView* KonqViewManager::splitView ( Qt::Orientation orientation,
                                            const KURL &url,
                                            QString serviceType,
                                            const QString &serviceName )
{
  kdDebug(1202) << "KonqViewManager::splitView(ServiceType)" << endl;

  QString locationBarURL;
  KonqFrame* viewFrame = 0L;
  if( m_pMainContainer )
  {
    viewFrame = m_pMainWindow->currentView()->frame();
    locationBarURL = m_pMainWindow->currentView()->locationBarURL();
  }

  KonqView* childView = split( viewFrame, orientation, serviceType, serviceName );

  if( childView )
  {
    childView->openURL( url );
    if( !locationBarURL.isEmpty() )
      childView->setLocationBarURL( locationBarURL );
  }

  return childView;
}

KonqView* KonqViewManager::splitWindow( Qt::Orientation orientation )
{
  kdDebug(1202) << "KonqViewManager::splitWindow(default)" << endl;

  KURL url = m_pMainWindow->currentView()->url();

  QString locationBarURL;
  KonqFrameBase* splitFrame = 0L;
  if( m_pMainContainer )
  {
    splitFrame = m_pMainContainer->firstChild();
    locationBarURL = m_pMainWindow->currentView()->locationBarURL();
  }

  KonqView* childView = split( splitFrame, orientation );

  if( childView )
  {
    childView->openURL( url );
    if( !locationBarURL.isEmpty() )
      childView->setLocationBarURL( locationBarURL );
  }

  return childView;
}

KonqView* KonqViewManager::split (KonqFrameBase* splitFrame,
                                       Qt::Orientation orientation,
                                       const QString &serviceType, const QString &serviceName,
                                       KonqFrameContainer **newFrameContainer )
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

    printSizeInfo( splitFrame, parentContainer, "before split");

    // done by viewCountChanged()
    //if ( splitFrame->widget()->inherits( "KonqFrame" ) )
    //  ((KonqFrame *)splitFrame->widget())->statusbar()->showStuff();

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
    newContainer->setUpdatesEnabled( false );
    newContainer->setOpaqueResize( true );
    newContainer->show();

    if( moveNewContainer )
      parentContainer->moveToFirst( newContainer );

    //kdDebug(1202) << "Move in child" << endl;
    splitFrame->widget()->reparent( newContainer, pos, true /*showIt*/ );

    printSizeInfo( splitFrame, parentContainer, "after reparent" );

    //kdDebug(1202) << "Create new child" << endl;
    childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    printSizeInfo( splitFrame, parentContainer, "after child insert" );

    splitFrame->widget()->setUpdatesEnabled( true );
    newContainer->setUpdatesEnabled( true );
    parentContainer->setUpdatesEnabled( true );

    if ( newFrameContainer )
      *newFrameContainer = newContainer;

  }
  else // We had no main container, create one
  {
    m_pMainContainer = new KonqFrameContainer( orientation, m_pMainWindow );
    m_pMainWindow->setView( m_pMainContainer );
    m_pMainContainer->setOpaqueResize();
    m_pMainContainer->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );

    childView = setupView( m_pMainContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    m_pMainContainer->show();

    //childView->frame()->statusbar()->hideStuff();

    if ( newFrameContainer )
      *newFrameContainer = m_pMainContainer;
  }

  printFullHierarchy( m_pMainContainer );

  return childView;
}

void KonqViewManager::removeView( KonqView *view )
{
  //kdDebug(1202) << "---------------- removeView" << view << endl;
  if ( activePart() == view->part() )
  {
    KonqView *nextView = chooseNextView( view );
    // Don't remove the last view
    if ( nextView == 0L )
      return;
    // Ensure this is not the active view anymore
    //kdDebug(1202) << "Setting part " << nextView->part() << " as active" << endl;
    nextView->part()->widget()->setFocus(); // Will set the part as active
    //setActivePart( nextView->part() );
  }

  KonqFrameContainer* parentContainer = view->frame()->parentContainer();
  KonqFrameContainer* grandParentContainer = parentContainer->parentContainer();
  //kdDebug(1202) << "view=" << view << " parentContainer=" << parentContainer
  //              << " grandParentContainer=" << grandParentContainer << endl;
  bool moveOtherChild = (grandParentContainer->idAfter( parentContainer ) != 0);

  KonqFrameBase* otherFrame = parentContainer->otherChild( view->frame() );

  if( otherFrame == 0L ) {
    warning("KonqViewManager::removeView: This shouldn't happen!");
    return;
  }

  QPoint pos = otherFrame->widget()->pos();

  otherFrame->reparentFrame( m_pMainWindow, pos );
  otherFrame->widget()->hide(); // Can't hide before, but after is better than nothing
  otherFrame->widget()->resize( 100, 30 ); // bring it to the QWidget defaultsize

  m_pMainWindow->removeChildView( view );

  parentContainer->removeChildFrame( view->frame() );
  //kdDebug(1202) << "Deleting view frame" << view->frame() << endl;
  delete view->frame();
  // This deletes the widgets inside, including the part's widget, so tell the child view
  view->partDeleted();
  //kdDebug(1202) << "Deleting view " << view << endl;
  delete view;

  grandParentContainer->removeChildFrame( parentContainer );
  //kdDebug(1202) << "Deleting parentContainer " << parentContainer
  //              << ". Its parent is " << parentContainer->parent() << endl;
  delete parentContainer;

  otherFrame->reparentFrame( grandParentContainer, pos, true/*showIt*/ );
  if( moveOtherChild )
    grandParentContainer->moveToFirst( otherFrame->widget() );

  //kdDebug(1202) << "------------- removeView done " << view << endl;
  printFullHierarchy( m_pMainContainer );
}

// reimplemented from PartManager
void KonqViewManager::removePart( KParts::Part * part )
{
  kdDebug(1202) << "KonqViewManager::removePart ( " << part << " )" << endl;
  // This is called when a part auto-deletes itself (case 1), or when
  // the "delete view" above deletes, in turn, the part (case 2)

  // If we were called by PartManager::slotObjectDestroyed, then the inheritance has
  // been deleted already... Can't use inherits().

  KonqView * cv = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(part) );
  if ( cv ) // the child view still exists, so we are in case 1
  {
      //kdDebug(1202) << "Found a child view" << endl;
      cv->partDeleted(); // tell the child view that the part auto-deletes itself
      removeView( cv );
      //kdDebug(1202) << "removeView returned" << endl;
  }

  //kdDebug(1202) << "Calling KParts::PartManager::removePart " << part << endl;
  KParts::PartManager::removePart( part );
  //kdDebug(1202) << "KonqViewManager::removePart ( " << part << " ) done" << endl;
}

void KonqViewManager::viewCountChanged()
{
  bool bShowActiveViewIndicator = ( m_pMainWindow->activeViewsCount() > 1 );
  bool bShowLinkedViewIndicator = ( m_pMainWindow->viewCount() > 1 );

  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  for (  ; it != end ; ++it )
  {
      it.data()->frame()->statusbar()->showActiveViewIndicator(
          bShowActiveViewIndicator && !it.data()->passiveMode()
      );
      it.data()->frame()->statusbar()->showLinkedViewIndicator( bShowLinkedViewIndicator );
  }

}

void KonqViewManager::saveViewProfile( KConfig &cfg )
{
  kdDebug(1202) << "KonqViewManager::saveViewProfile" << endl;
  if( m_pMainContainer->firstChild() ) {
    cfg.writeEntry( "RootItem", m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 ) );
    QString prefix = m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 );
    prefix.append( '_' );
    m_pMainContainer->firstChild()->saveConfig( &cfg, prefix, 0, 1 );
  }

  cfg.sync();
}

void KonqViewManager::loadViewProfile( KConfig &cfg )
{
  kdDebug(1202) << "KonqViewManager::loadViewProfile" << endl;
  clear();

  QString rootItem = cfg.readEntry( "RootItem" );

  if( rootItem.isNull() ) {
    // Config file doesn't contain anything about view profiles, fallback to defaults
    rootItem = "InitialView";
  }

  kdDebug(1202) << "Load RootItem " << rootItem << endl;

  m_pMainContainer = new KonqFrameContainer( Qt::Horizontal, m_pMainWindow );
  m_pMainWindow->setView( m_pMainContainer );
  m_pMainContainer->setOpaqueResize();
  m_pMainContainer->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );
  m_pMainContainer->show();

  loadItem( cfg, m_pMainContainer, rootItem );

  KonqView *nextChildView = chooseNextView( 0L );
  setActivePart( nextChildView ? nextChildView->part() : 0L );

  kdDebug(1202) << "KonqViewManager::loadViewProfile done" << endl;
  printFullHierarchy( m_pMainContainer );
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainer *parent,
				const QString &name )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + '_';

  kdDebug(1202) << "begin loadItem: " << name << endl;

  if( name.find("View") != -1 ) {
    kdDebug(1202) << "Item is View" << endl;
    //load view config
    QString url = cfg.readEntry( QString::fromLatin1( "URL" ).prepend( prefix ), QDir::homeDirPath() );

    if ( url == "file:$HOME" ) // HACK
      url = QDir::homeDirPath().prepend( "file:" );

    kdDebug(1202) << "URL: " << url << endl;
    QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), "inode/directory");
    kdDebug(1202) << "ServiceType: " << serviceType << endl;

    QString serviceName = cfg.readEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ) );

    bool passiveMode = cfg.readBoolEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), false );
    bool linkedView = cfg.readBoolEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), false );

    KService::Ptr service;
    KTrader::OfferList partServiceOffers, appServiceOffers;

    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers );
    if ( viewFactory.isNull() )
    {
      warning("Profile Loading Error: View creation failed" );
      return; //ugh..
    }

    kdDebug(1202) << "Creating View Stuff" << endl;
    KonqView *childView = setupView( parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    childView->setPassiveMode( passiveMode );
    childView->setLinkedView( linkedView );

    //QCheckBox *checkBox = childView->frame()->statusbar()->passiveModeCheckBox();
    //checkBox->setChecked( passiveMode );

    childView->openURL( KURL( url ) );
    childView->setLocationBarURL( url );
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
      loadItem( cfg, parent, "InitialView" );
    }
    else
    {
      KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent );
      newContainer->setOpaqueResize();
      newContainer->show();

      loadItem( cfg, newContainer, childList.at(0) );
      loadItem( cfg, newContainer, childList.at(1) );

      newContainer->setSizes( sizes );
    }
  }
  else
      kdWarning() << "Profile Loading Error: Unknown item " << name;

  kdDebug(1202) << "end loadItem: " << name << endl;
}

void KonqViewManager::clear()
{
  kdDebug(1202) << "KonqViewManager::clear: " << endl;
  QList<KonqView> viewList;
  QListIterator<KonqView> it( viewList );

  if (m_pMainContainer) {
    m_pMainContainer->listViews( &viewList );

    for ( it.toFirst(); it.current(); ++it ) {
      m_pMainWindow->removeChildView( it.current() );
      delete it.current();
    }

    //kdDebug(1202) << "deleting m_pMainContainer " << endl;
    delete m_pMainContainer;
    m_pMainContainer = 0L;
  }
}

KonqView *KonqViewManager::chooseNextView( KonqView *view )
{
  kdDebug(1202) << "KonqViewManager(" << this << ")::chooseNextView(" << view << ")" << endl;
  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();

  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  if ( view ) // find it in the map - can't use the key since view->part() might be 0L
      while ( it != end && it.data() != view )
          ++it;

  KonqMainWindow::MapViews::Iterator startIt = it;

  // the view should always be in the list, shouldn't it?
  // maybe use assert( it != end )?
   if ( it == end ) {
       kdWarning() << "View " << view << " is not in list !" << endl;
     it = mapViews.begin();
     if ( it == end )
       return 0L; // wow, that certainly caught all possibilities!
   }

  while ( true )
  {

    if ( ++it == end ) // move to next
      it = mapViews.begin(); // rewind on end
 	
    if ( it == startIt )
      break; // no next view found

    KonqView *nextView = it.data();
    if ( nextView && !nextView->passiveMode() )
      return nextView;
  }

  return 0L; // no next view found
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

void KonqViewManager::slotProfileDlg()
{
  KonqProfileDlg dlg( this, m_pMainWindow );
  dlg.exec();
  profileListDirty();
}

void KonqViewManager::profileListDirty()
{
  kdDebug(1202) << "KonqViewManager::profileListDirty()" << endl;
  m_bProfileListDirty = true;
  QStringList profiles = KonqFactory::instance()->dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
  if ( m_pamProfiles )
      m_pamProfiles->setEnabled( profiles.count() > 0 );
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

    viewFactory = KonqFactory::createView( cv->serviceType(), cv->service()->name(),
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
					   const QString &serviceType )
{
  kdDebug(1202) << "KonqViewManager::setupView" << endl;

  QString sType = serviceType;

  if ( sType.isEmpty() )
    sType = m_pMainWindow->currentView()->serviceType();

  KonqFrame* newViewFrame = new KonqFrame( parentContainer, "KonqFrame" );

  kdDebug(1202) << "Creating KonqView" << endl;
  KonqView *v = new KonqView( viewFactory, newViewFrame,
                              m_pMainWindow, service, partServiceOffers, appServiceOffers, sType );
  kdDebug(1202) << "KonqView created" << endl;

  QObject::connect( v, SIGNAL( sigPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ),
                    m_pMainWindow, SLOT( slotPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ) );

  m_pMainWindow->insertChildView( v );

  newViewFrame->show();

  addPart( v->part(), false );

  kdDebug(1202) << "KonqViewManager::setupView done" << endl;
  return v;
}

void KonqViewManager::slotProfileActivated( int id )
{

  QMap<QString, QString>::ConstIterator nameIt = m_mapProfileNames.find( m_pamProfiles->popupMenu()->text( id ) );
  if ( nameIt == m_mapProfileNames.end() )
    return;

  KConfig cfg( KIO::encodeFileName( *nameIt ), true );
  cfg.setGroup( "Profile" );
  loadViewProfile( cfg );
}

void KonqViewManager::slotProfileListAboutToShow()
{
  if ( !m_pamProfiles || !m_bProfileListDirty )
    return;

  QPopupMenu *popup = m_pamProfiles->popupMenu();

  popup->clear();
  m_mapProfileNames.clear();

  QStringList profiles = KonqFactory::instance()->dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
  QStringList::ConstIterator pIt = profiles.begin();
  QStringList::ConstIterator pEnd = profiles.end();
  for (; pIt != pEnd; ++pIt )
  {
    QFileInfo info( *pIt );
    QString profileName = KIO::decodeFileName( info.baseName() );
    KSimpleConfig cfg( *pIt, true );
    cfg.setGroup( "Profile" );
    if ( cfg.hasKey( "Name" ) )
      profileName = cfg.readEntry( "Name" );

    m_mapProfileNames.insert( profileName, *pIt );
    popup->insertItem( profileName );
  }

  m_bProfileListDirty = false;
}

void KonqViewManager::printSizeInfo( KonqFrameBase* frame,
				     KonqFrameContainer* parent,
				     const char* msg )
{
  QRect r;
  r = frame->widget()->geometry();
  debug("Child size %s : x: %d, y: %d, w: %d, h: %d", msg, r.x(),r.y(),r.width(),r.height() );

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
            kdDebug(1202) << spaces << "  Null child ! " << endl;
        else if ( child->widget()->isA("KonqFrameContainer") )
            printFullHierarchy( static_cast<KonqFrameContainer *>(child), ident + 2 );
        else
            kdDebug(1202) << spaces << "  " << "KonqFrame " << child << " containing a "
                          << static_cast<KonqFrame *>(child)->part()->widget()->className() << endl;
        child = container->secondChild();
        if ( !child )
            kdDebug(1202) << spaces << "  Null child ! " << endl;
        else if ( child->widget()->isA("KonqFrameContainer") )
            printFullHierarchy( static_cast<KonqFrameContainer *>(child), ident + 2 );
        else
            kdDebug(1202) << spaces << "  " << "KonqFrame " << child << " containing a "
                          << static_cast<KonqFrame *>(child)->part()->widget()->className() << endl;
    }
    else
        kdDebug(1202) << "Null container ?!?!" << endl;
}

#include "konq_viewmgr.moc"
