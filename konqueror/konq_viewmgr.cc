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
#include "konq_mainview.h"
#include "konq_childview.h"
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

#include <assert.h>

template class QList<KonqChildView>;

KonqViewManager::KonqViewManager( KonqMainView *mainView )
 : KParts::PartManager( mainView )
{
  m_pMainView = mainView;

  m_pMainContainer = 0L;

  m_bProfileListDirty = true;

  // we need this for a proper layout of the toolbars upon startup, when no real view has been loaded, yet
  m_dummyWidget = new QWidget( mainView );
  mainView->setView( m_dummyWidget );
}

KonqViewManager::~KonqViewManager()
{
  clear();

  if (m_pMainContainer) delete m_pMainContainer;
}

KParts::ReadOnlyPart* KonqViewManager::splitView ( Qt::Orientation orientation )
{
  kdDebug(1202) << "KonqViewManager::splitView(default)" << endl;

  return splitView( orientation, m_pMainView->currentChildView()->url() );
}

KParts::ReadOnlyPart* KonqViewManager::splitView ( Qt::Orientation orientation,
						   const KURL &url,
						   QString serviceType )
{
  kdDebug(1202) << "KonqViewManager::splitView(ServiceType)" << endl;

  QString locationBarURL;
  KonqFrame* viewFrame = 0L;
  if( m_pMainContainer )
  {
    viewFrame = m_pMainView->currentChildView()->frame();
    locationBarURL = m_pMainView->currentChildView()->locationBarURL();
  }

  KParts::ReadOnlyPart* view = split( viewFrame, orientation, serviceType );

  if( view )
  {
    KonqChildView * cv = m_pMainView->childView( view );
    cv->openURL( url );
    if( !locationBarURL.isEmpty() )
      cv->setLocationBarURL( locationBarURL );
  }

  return view;
}

KParts::ReadOnlyPart* KonqViewManager::splitWindow( Qt::Orientation orientation )
{
  kdDebug(1202) << "KonqViewManager::splitWindow(default)" << endl;

  KURL url = m_pMainView->currentChildView()->url();

  QString locationBarURL;
  KonqFrameBase* splitFrame = 0L;
  if( m_pMainContainer )
  {
    splitFrame = m_pMainContainer->firstChild();
    locationBarURL = m_pMainView->currentChildView()->locationBarURL();
  }

  KParts::ReadOnlyPart* view = split( splitFrame, orientation );

  if( view )
  {
    KonqChildView * cv = m_pMainView->childView( view );
    cv->openURL( url );
    if( !locationBarURL.isEmpty() )
      cv->setLocationBarURL( locationBarURL );
  }

  return view;
}

KParts::ReadOnlyPart* KonqViewManager::split (KonqFrameBase* splitFrame,
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

  KParts::ReadOnlyPart *view = 0L;

  if( m_pMainContainer )
  {
    assert( splitFrame );

    KonqFrameContainer* parentContainer = splitFrame->parentContainer();
    bool moveNewContainer = (parentContainer->idAfter( splitFrame->widget() ) != 0);

    printSizeInfo( splitFrame, parentContainer, "before split");

    if ( splitFrame->widget()->inherits( "KonqFrame" ) )
      ((KonqFrame *)splitFrame->widget())->statusbar()->showStuff();

    splitFrame->widget()->setUpdatesEnabled( false );
    parentContainer->setUpdatesEnabled( false );

    QPoint pos = splitFrame->widget()->pos();

    debug("Move out child");
    //splitFrame->widget()->hide();
    splitFrame->widget()->reparent( m_pMainView, 0, pos );
    splitFrame->widget()->resize( 100, 30 ); // bring it to the QWidget defaultsize, so that both container childs are equally size after split

    debug("Create new Container");
    KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer );
    newContainer->setUpdatesEnabled( false );
    newContainer->setOpaqueResize( true );
    newContainer->show();

    if( moveNewContainer )
      parentContainer->moveToFirst( newContainer );

    debug("Move in Child");
    splitFrame->widget()->reparent( newContainer, 0, pos, true );

    printSizeInfo( splitFrame, parentContainer, "after reparent" );

    debug("Create new Child");
    KonqChildView *childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    view = childView->view();

    printSizeInfo( splitFrame, parentContainer, "after child insert" );

    splitFrame->widget()->setUpdatesEnabled( true );
    newContainer->setUpdatesEnabled( true );
    parentContainer->setUpdatesEnabled( true );

    if ( newFrameContainer )
      *newFrameContainer = newContainer;
  }
  else {
    m_pMainContainer = new KonqFrameContainer( orientation, m_pMainView );
    m_pMainView->setView( m_pMainContainer );
    m_pMainContainer->setOpaqueResize();
    m_pMainContainer->setGeometry( 0, 0, m_pMainView->width(), m_pMainView->height() );

    KonqChildView *childView = setupView( m_pMainContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    if ( m_dummyWidget )
      delete (QWidget *)m_dummyWidget;

    // exclude the splitter and all child widgets from the part focus handling
    m_pMainContainer->show();

    childView->frame()->statusbar()->hideStuff();

    if ( newFrameContainer )
      *newFrameContainer = m_pMainContainer;

    view = childView->view();
  }

  return view;
}

void KonqViewManager::removeView( KonqChildView *view )
{
  if ( activePart() == view->view() )
  {
    KonqChildView *nextView = chooseNextView( view );
    // Don't remove the last view
    if ( nextView == 0L )
      return;
    // Ensure this is not the active view anymore
    setActivePart( nextView->view() );
  }

  KonqFrameContainer* parentContainer = view->frame()->parentContainer();
  KonqFrameContainer* grandParentContainer = parentContainer->parentContainer();
  bool moveOtherChild = (grandParentContainer->idAfter( parentContainer ) != 0);

  KonqFrameBase* otherFrame = parentContainer->otherChild( view->frame() );

  if( otherFrame == 0L ) {
    warning("KonqViewManager::removeView: This shouldn't happen!");
    return;
  }

  QPoint pos = otherFrame->widget()->pos();

  //otherFrame->widget()->hide();
  otherFrame->reparent( m_pMainView, 0, pos );
  otherFrame->widget()->resize( 100, 30 ); // bring it to the QWidget defaultsize

  m_pMainView->removeChildView( view );

  //triggering QObject::childEvent manually
  //  parentContainer->removeChild( view->frame() );
  delete view;

  //triggering QObject::childEvent manually
  grandParentContainer->removeChild( parentContainer );
  delete parentContainer;

  otherFrame->reparent( grandParentContainer, 0, pos, true );
  if( moveOtherChild )
    grandParentContainer->moveToFirst( otherFrame->widget() );

  if(m_pMainView->viewList().count() == 1)
    m_pMainView->currentChildView()->frame()->statusbar()->hideStuff();
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

  kdDebug(1202) << "Load RootItem " << debugString(rootItem) << endl;

  m_pMainContainer = new KonqFrameContainer( Qt::Horizontal, m_pMainView );
  m_pMainView->setView( m_pMainContainer );
  m_pMainContainer->setOpaqueResize();
  m_pMainContainer->setGeometry( 0, 0, m_pMainView->width(), m_pMainView->height() );
  m_pMainContainer->show();

  if ( m_dummyWidget )
    delete (QWidget *)m_dummyWidget;

  loadItem( cfg, m_pMainContainer, rootItem );

  QValueList<KParts::ReadOnlyPart *> lst = m_pMainView->viewList();

  KParts::ReadOnlyPart *view = *lst.begin();
  KonqChildView *childView = m_pMainView->childView( view );

  if ( childView->passiveMode() )
  {
    KonqChildView *nextChildView = chooseNextView( childView );
    setActivePart( nextChildView->view() );
    if ( lst.count() == 2 )
      nextChildView->frame()->statusbar()->hideStuff();
  }
  else
    setActivePart( view );

  if ( lst.count() == 1 && !childView->passiveMode() )
    childView->frame()->statusbar()->hideStuff();
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainer *parent,
				const QString &name )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + '_';

  kdDebug(1202) << "begin loadItem: " << debugString(name) << endl;

  if( name.find("View") != -1 ) {
    kdDebug(1202) << "Item is View" << endl;
    //load view config
    QString url = cfg.readEntry( QString::fromLatin1( "URL" ).prepend( prefix ), QDir::homeDirPath() );

    if ( url == "file:$HOME" ) // HACK
      url = QDir::homeDirPath().prepend( "file:" );

    kdDebug(1202) << "URL: " << debugString(url) << endl;
    QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), "inode/directory");
    kdDebug(1202) << "ServiceType: " << debugString(serviceType) << endl;

    QString serviceName = cfg.readEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ) );

    bool passiveMode = cfg.readBoolEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), false );

    KService::Ptr service;
    KTrader::OfferList partServiceOffers, appServiceOffers;

    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers );
    if ( viewFactory.isNull() )
    {
      warning("Profile Loading Error: View creation failed" );
      return; //ugh..
    }

    kdDebug(1202) << "Creating View Stuff" << endl;
    KonqChildView *childView = setupView( parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType );

    childView->setPassiveMode( passiveMode );

    QCheckBox *checkBox = childView->frame()->statusbar()->passiveModeCheckBox();

    checkBox->setChecked( passiveMode );

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
      warning("Profile Loading Error: No orientation specified in %s", debugString(name));;
      o = Qt::Horizontal;
    }

    QValueList<int> sizes =
	cfg.readIntListEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ));

    QStrList childList;
    if( cfg.readListEntry( QString::fromLatin1( "Children" ).prepend( prefix ), childList ) < 2 )
    {
      warning("Profile Loading Error: Less than two children in %s", debugString(name));
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
    warning("Profile Loading Error: Unknown item %s", debugString(name));

  kdDebug(1202) << "end loadItem: " << debugString(name) << endl;
}

void KonqViewManager::clear()
{
  QList<KonqChildView> viewList;
  QListIterator<KonqChildView> it( viewList );

  if (m_pMainContainer) {
    m_pMainContainer->listViews( &viewList );

    for ( it.toFirst(); it.current(); ++it ) {
      m_pMainView->removeChildView( it.current() );
      delete it.current();
    }

    delete m_pMainContainer;
    m_pMainContainer = 0L;
  }
}

KonqChildView *KonqViewManager::chooseNextView( KonqChildView *view )
{
  QValueList<KParts::ReadOnlyPart *> viewList = m_pMainView->viewList();

  if ( !view )
    return m_pMainView->childView( *viewList.begin() );

  QValueList<KParts::ReadOnlyPart *>::ConstIterator it = viewList.find( view->view() );
  QValueList<KParts::ReadOnlyPart *>::ConstIterator end = viewList.end();

  QValueList<KParts::ReadOnlyPart *>::ConstIterator startIt = it;

  // the view should always be in the list, shouldn't it?
  // maybe use assert( it != end )?
   if ( it == end ) {
     it = viewList.begin();
      if ( it == end )
       return 0L; // wow, that certainly caught all possibilities!
   }

  while ( true )
  {

    if ( it++ == end )
       it = viewList.begin();
 	
    if ( it == startIt )
       break;

    KonqChildView *nextView = m_pMainView->childView( *it );
    if ( nextView && !nextView->passiveMode() )
      return nextView;

  }

  return 0L; // ARGHL

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
	
  m_bProfileListDirty = true;
}

void KonqViewManager::slotProfileDlg()
{
  KonqProfileDlg dlg( this, m_pMainView );
  dlg.exec();
  m_bProfileListDirty = true;
}

KonqViewFactory KonqViewManager::createView( const QString &serviceType,
					  const QString &serviceName,
					  KService::Ptr &service,
					  KTrader::OfferList &partServiceOffers,
					  KTrader::OfferList &appServiceOffers )
{
  kdDebug(1202) << "KonqViewManager::createView" << endl;
  KonqViewFactory viewFactory;

  if( serviceType.isEmpty() && m_pMainView->currentChildView() ) {
    //clone current view
    KonqChildView *cv = m_pMainView->currentChildView();

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

KonqChildView *KonqViewManager::setupView( KonqFrameContainer *parentContainer,
                                           KonqViewFactory &viewFactory,
				           const KService::Ptr &service,
				           const KTrader::OfferList &partServiceOffers,
					   const KTrader::OfferList &appServiceOffers,
					   const QString &serviceType )
{
  kdDebug(1202) << "KonqViewManager::setupView" << endl;

  QString sType = serviceType;

  if ( sType.isEmpty() )
    sType = m_pMainView->currentChildView()->serviceType();

  KonqFrame* newViewFrame = new KonqFrame( parentContainer, "KonqFrame" );

  kdDebug(1202) << "Creating KonqChildView" << endl;
  KonqChildView *v = new KonqChildView( viewFactory, newViewFrame,
					m_pMainView, service, partServiceOffers, appServiceOffers, sType );
  kdDebug(1202) << "KonqChildView created" << endl;

  QObject::connect( v, SIGNAL( sigViewChanged( KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ),
                    m_pMainView, SLOT( slotViewChanged( KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ) );

  m_pMainView->insertChildView( v );

  newViewFrame->show();

  addPart( v->view(), false );

  kdDebug(1202) << "KonqViewManager::setupView done" << endl;
  return v;
}

void KonqViewManager::slotProfileActivated( int id )
{

  QMap<QString, QString>::ConstIterator nameIt = m_mapProfileNames.find( m_pamProfiles->popupMenu()->text( id ) );
  if ( nameIt == m_mapProfileNames.end() )
    return;

  KConfig cfg( *nameIt, true );
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
    QString profileName = info.baseName();
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

#include "konq_viewmgr.moc"
