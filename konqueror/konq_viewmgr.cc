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

#include "konq_viewmgr.h"
#include "konq_mainview.h"
#include "konq_childview.h"
#include "konq_factory.h"
#include "konq_frame.h"

#include <qstringlist.h>
#include <qdir.h>

#include <kconfig.h>
#include <browser.h>

#include <opApplication.h>

KonqViewManager::KonqViewManager( KonqMainView *mainView )
{
  m_pMainView = mainView;

  m_pMainContainer = 0L;
}

KonqViewManager::~KonqViewManager()
{
  clear();

  m_pMainView->removeSeparatedWidget( m_pMainContainer );
  delete m_pMainContainer;
}

void KonqViewManager::splitView ( Qt::Orientation orientation ) 
{
  kdebug(0, 1202, "KonqViewManager::splitview(default)" );
  KonqChildView* currentChildView = m_pMainView->currentChildView();
  QString url = currentChildView->url();
  const QString serviceType = currentChildView->serviceTypes().first();

  Browser::View_var vView;
  QStringList serviceTypes;
  
  // KonqFactory::createView() ignores this if the servicetypes is
  // not inode/directory
  Konqueror::DirectoryDisplayMode dirMode = Konqueror::LargeIcons;
  
  if ( currentChildView->supportsServiceType( "inode/directory" ) )
  {
    if ( currentChildView->view()->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
      dirMode = Konqueror::TreeView;
    else
    {
      Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( currentChildView->view() );
      dirMode = iv->viewMode();
    }
  }
  
  if ( CORBA::is_nil( ( vView = KonqFactory::createView( serviceType, serviceTypes, m_pMainView, dirMode ) ) ) )
    return; //do not split the view at all if we can't clone the current view

  splitView( orientation, vView, serviceTypes );

  m_pMainView->childView( vView->id() )->openURL( url );
}

void KonqViewManager::splitView ( Qt::Orientation orientation, 
				  Browser::View_ptr newView,
				  const QStringList &newViewServiceTypes)
{
  kdebug(0, 1202, "KonqViewManager::splitview" );
  
  if( m_pMainContainer )
  {
    KonqFrame* splitFrame;
    KonqChildView* currentChildView = m_pMainView->currentChildView();
    splitFrame = currentChildView->frame();
    
    KonqFrameContainer* parentContainer = splitFrame->parentContainer();
    bool moveNewContainer = (parentContainer->idAfter( splitFrame ) != 0);

    if( moveNewContainer )       
      kdebug(0, 1202, "Move new splitter: Yes %d",parentContainer->idAfter( splitFrame ) );
    else
      kdebug(0, 1202, "Move new splitter: No %d",parentContainer->idAfter( splitFrame ) );
    
    QValueList<int> sizes;
    sizes = parentContainer->sizes();

    QPoint pos = splitFrame->pos();

    splitFrame->hide();
    splitFrame->reparent( m_pMainView, 0, pos );
    
    KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer );
    if( moveNewContainer )
      parentContainer->moveToFirst( newContainer );

    splitFrame->reparent( newContainer, 0, pos, true );

    setupView( newContainer, newView, newViewServiceTypes );

    //have to to something about the flickering. Well, actually these 
    //adjustments shouldn't be neccessary. I'm not sure wether this is a 
    //QSplitter problem or not. Michael.
    QValueList<int> sizesNew;
    sizesNew.append(100);
    sizesNew.append(100);
    newContainer->setSizes( sizesNew );

    parentContainer->setSizes( sizes );

    newContainer->show();

  }
  else {
    m_pMainContainer = new KonqFrameContainer( orientation, m_pMainView );
    m_pMainContainer->setOpaqueResize();

    setupView( m_pMainContainer, newView, newViewServiceTypes );

    // exclude the splitter and all child widgets from the part focus handling
    m_pMainView->addSeparatedWidget( m_pMainContainer );
    m_pMainContainer->show();
  }
}

void KonqViewManager::removeView( KonqChildView *view )
{
  KonqFrameContainer* parentContainer = view->frame()->parentContainer();
  KonqFrameContainer* grandParentContainer = parentContainer->parentContainer();
  bool moveOtherChild = (grandParentContainer->idAfter( parentContainer ) != 0);

  KonqFrameBase* otherFrame = parentContainer->otherChild( view->frame() );

  if( otherFrame == 0L ) {
    warning("KonqViewManager::removeView: This shouldn't happen!");
    return;
  }
  
  QPoint pos = otherFrame->widget()->pos();

  QValueList<int> sizes;
  sizes = grandParentContainer->sizes();

  //otherFrame->widget()->hide();
  otherFrame->reparent( m_pMainView, 0, pos );

  m_pMainView->removeChildView( view->id() );

  //temporarily commented out. Michael.
  //if ( isLinked( view ) )
  //  removeLink( view );

  //triggering QObject::childEvent manually
  parentContainer->removeChild( view->frame() );
  delete view;

  //triggering QObject::childEvent manually
  grandParentContainer->removeChild( parentContainer );
  delete parentContainer;

  otherFrame->reparent( grandParentContainer, 0, pos, true );
  if( moveOtherChild )
    grandParentContainer->moveToFirst( otherFrame->widget() );

  //have to to something about the flickering. Well, actually these 
  //adjustments shouldn't be neccessary. I'm not sure wether this is a 
  //QSplitter problem or not. Michael.
  grandParentContainer->setSizes( sizes );
}

void KonqViewManager::saveViewProfile( KConfig &cfg )
{
  kdebug(0, 1202, "KonqViewManager::saveViewProfile");  
  if( m_pMainContainer->firstChild() ) {
    cfg.writeEntry( "RootItem", m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 ) );
    cfg.setGroup(  m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 ) );
    m_pMainContainer->firstChild()->saveConfig( &cfg, 0, 1 );
  } 
  
  cfg.sync();
}

void KonqViewManager::loadViewProfile( KConfig &cfg )
{
  kdebug(0, 1202, "KonqViewManager::loadViewProfile");  
  clear();

  QString rootItem = cfg.readEntry( "RootItem" );

  if( rootItem.isNull() ) {
    // Config file doesn't contain anything about view profiles, fallback to defaults
    rootItem = "InitialView";
  }

  kdebug(0, 1202, "Load RootItem %s", rootItem.data());  

  m_pMainContainer = new KonqFrameContainer( Qt::Horizontal, m_pMainView );
  m_pMainContainer->setOpaqueResize();
  m_pMainContainer->setGeometry( 0, 0, m_pMainView->width(), m_pMainView->height() );
  m_pMainContainer->show();

  loadItem( cfg, m_pMainContainer, rootItem );

  OpenParts::MainWindow_var vMainWindow = m_pMainView->mainWindow();
  
  if ( vMainWindow->activePartId() == m_pMainView->id() )
    vMainWindow->setActivePart( viewIdByNumber( 0 ) );
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainer *parent, 
				const QString &name )
{
  if( name != "InitialView" )
    cfg.setGroup( name );
  kdebug(0, 1202, "begin loadItem: %s",name.data() );  

  if( name.find("View") != -1 ) {
    kdebug(0, 1202, "Item is View");  
    //load view config
    QString url = cfg.readEntry( QString::fromLatin1( "URL" ), QDir::homeDirPath() );
    kdebug(0, 1202, "URL: %s",url.data());  
    QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ), "inode/directory");
    kdebug(0, 1202, "ServiceType: %s", serviceType.data());  

    Konqueror::DirectoryDisplayMode dirMode = Konqueror::LargeIcons;

    if ( serviceType == "inode/directory" )
    {
      QString strDirMode = cfg.readEntry( QString::fromLatin1( "DirectoryMode" ), "LargeIcons" );

      if ( strDirMode == "LargeIcons" )
	dirMode = Konqueror::LargeIcons;
      else if ( strDirMode == "SmallIcons" )
	dirMode = Konqueror::SmallIcons;
      else if ( strDirMode == "SmallVerticalIcons" )
	dirMode = Konqueror::SmallVerticalIcons;
      else if ( strDirMode == "TreeView" )
	dirMode = Konqueror::TreeView;
      else assert( 0 );
	
    }

    Browser::View_var vView;
    QStringList serviceTypes;

    //Simon TODO: error handling
    vView = KonqFactory::createView( serviceType, serviceTypes, m_pMainView, dirMode);
    if( !CORBA::is_nil( vView )	) {
      kdebug(0, 1202, "Creating View Stuff");  
      setupView( parent, vView, serviceTypes);

      m_pMainView->childView( vView->id() )->openURL( url );
    }
    else
      warning("Profile Loading Error: View creation failed" );
  }
  else if( name.find("Container") != -1 ) {
    kdebug(0, 1202, "Item is Container");  

    //load container config
    QString ostr = cfg.readEntry( QString::fromLatin1( "Orientation" ));
    kdebug(0, 1202, "Orientation: ",ostr.data());  
    Qt::Orientation o;
    if( ostr == "Vertical" )
      o = Qt::Vertical;
    else if( ostr == "Horizontal" )
      o = Qt::Horizontal;
    else {
      warning("Profile Loading Error: No orientation specified in %s", name.data());
      o = Qt::Horizontal;
    }

    QValueList<int> sizes = 
      QVariant( cfg.readPropertyEntry( QString::fromLatin1( "SplitterSizes" ), 
				       QVariant::IntList ) ).intListValue();

    QStrList childList;
    if( cfg.readListEntry( QString::fromLatin1( "Children" ), childList ) < 2 )
      warning("Profile Loading Error: Less than two children in %s", name.data());

    KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent );
    newContainer->setOpaqueResize();
    newContainer->show();

    loadItem( cfg, newContainer, childList.at(0) );
    loadItem( cfg, newContainer, childList.at(1) );

    newContainer->setSizes( sizes );
  }  
  else
    warning("Profile Loading Error: Unknown item %s", name.data());

  kdebug(0, 1202, "end loadItem: %s",name.data());  
}

void KonqViewManager::clear()
{
  QList<KonqChildView> viewList;
  QListIterator<KonqChildView> it( viewList );

  if (m_pMainContainer) {
    m_pMainContainer->listViews( &viewList );

    for ( ; it.current(); ++it ) {
      m_pMainView->removeChildView( it.current()->id() );
      delete it.current();
    }    
  
    delete m_pMainContainer;
    m_pMainContainer = 0L;
  }
}

void KonqViewManager::doGeometry( int width, int height )
{
  m_pMainContainer->setGeometry( 0, 0, width, height );
}

KonqChildView *KonqViewManager::chooseNextView( KonqChildView *view )
{
  QList<KonqChildView> viewList;

  m_pMainContainer->listViews( &viewList );

  if ( !view )
    return viewList.first();

  int pos = viewList.find( view ) + 1;
  if( pos >= viewList.count() )
    pos = 0;
  kdebug(0, 1202, "next view: %d", pos );  

  return viewList.at( pos );
}

unsigned long KonqViewManager::viewIdByNumber( int number )
{
  kdebug(0, 1202, "KonqViewManager::viewIdByNumber( %d )", number );  
  QList<KonqChildView> viewList;

  m_pMainContainer->listViews( &viewList );

  KonqChildView* view = viewList.at( number );
  if( view != 0L )
    return view->id();
  else
    return 0L;
}

//temporarily commented out. Michael.
/*
void KonqViewManager::createLink( KonqChildView *source, KonqChildView *destination )
{
  if ( isLinked( source ) )
    removeLink( source );
    
  m_mapViewLinks.insert( source, destination );
}

KonqChildView *KonqViewManager::readLink( KonqChildView *view )
{
  QMap<KonqChildView *, KonqChildView*>::ConstIterator it = m_mapViewLinks.find( view );
  
  if ( it != m_mapViewLinks.end() )
    return it.data();

  return view;
}

void KonqViewManager::removeLink( KonqChildView *source )
{
  m_mapViewLinks.remove( source );
}

KonqChildView *KonqViewManager::linkableViewVertical( KonqChildView *view, bool above )
{
  if ( m_lstRows.count() == 1 )
    return 0L;

  Konqueror::RowInfo *savedCurrentRow = m_lstRows.current();
  
  m_lstRows.findRef( view->rowInfo() );
  
  Konqueror::RowInfo *destRow = 0L;
  if ( above )
    destRow = m_lstRows.prev();
  else
    destRow = m_lstRows.next();

  m_lstRows.findRef( savedCurrentRow );
  
  if ( !destRow )
    return 0L;

  if ( destRow->children.count() != 1 )
    return 0L;

  return destRow->children.getFirst();
}

KonqChildView *KonqViewManager::linkableViewHorizontal( KonqChildView *view, bool left )
{
  Konqueror::RowInfo *row = view->rowInfo();
  
  if ( row->children.count() == 1 )
    return 0L;
    
  KonqChildView *savedCurrentView = row->children.current();
  
  row->children.findRef( view );
  
  KonqChildView *dest = 0L;
  
  if ( left )
    dest = row->children.prev();
  else
    dest = row->children.next();
    
  row->children.findRef( savedCurrentView );
  
  return dest;
}
*/
void KonqViewManager::setupView( KonqFrameContainer *parentContainer, Browser::View_ptr view, const QStringList &serviceTypes )
{
  KonqFrame* newViewFrame = new KonqFrame( parentContainer );

  KonqChildView *v = new KonqChildView( view, newViewFrame, 
					m_pMainView, serviceTypes );

  QObject::connect( v, SIGNAL( sigIdChanged( KonqChildView *, OpenParts::Id, OpenParts::Id ) ), 
                    m_pMainView, SLOT( slotIdChanged( KonqChildView * , OpenParts::Id, OpenParts::Id ) ) );

  m_pMainView->insertChildView( v );

  v->lockHistory();

  //if (isVisible()) v->show();
}
