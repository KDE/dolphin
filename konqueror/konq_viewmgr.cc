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
#include "konq_profiledlg.h"
#include "konq_iconview.h"
#include "browser.h"

#include <qstringlist.h>
#include <qdir.h>
#include <qevent.h>
#include <qapplication.h>

#include <kconfig.h>
#include <kstddirs.h>
#include <kdebug.h>

#include <assert.h>

KonqViewManager::KonqViewManager( KonqMainView *mainView )
{
  m_pMainView = mainView;

  m_pMainContainer = 0L;
  
  m_bProfileListDirty = true;
  
  qApp->installEventFilter( this );
}

KonqViewManager::~KonqViewManager()
{
  clear();

  if (m_pMainContainer) delete m_pMainContainer;
}

void KonqViewManager::splitView ( Qt::Orientation orientation ) 
{
  kdebug(0, 1202, "KonqViewManager::splitview(default)" );
  KonqChildView* currentChildView = m_pMainView->currentChildView();
  QString url = currentChildView->url();
  const QString serviceType = currentChildView->serviceTypes().first();

  BrowserView *pView;
  QStringList serviceTypes;
  
  // KonqFactory::createView() ignores this if the servicetypes is
  // not inode/directory
  Konqueror::DirectoryDisplayMode dirMode = Konqueror::LargeIcons;
  
  if ( currentChildView->supportsServiceType( "inode/directory" ) )
  {
//    if ( currentChildView->view()->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
    if ( currentChildView->view()->inherits( "KonqTreeView" ) )
      dirMode = Konqueror::TreeView;
    else
      dirMode = ((KonqKfmIconView *)currentChildView->view())->viewMode();
//    {
//      Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( currentChildView->view() );
//      KonqKfmIconView *iconView = (KonqKfmIconView *)(currentChildView->view());
      
//      dirMode = iconView->viewMode();
//#warning FIXME (Simon)
//        dirMode = Konqueror::LargeIcons;
//    }
  }
  
  if ( !( pView = KonqFactory::createView( serviceType, serviceTypes, dirMode ) ) )
    return; //do not split the view at all if we can't clone the current view

  splitView( orientation, pView, serviceTypes );

//  m_pMainView->childView( pView )->openURL( url );
  pView->openURL( url );
}

void KonqViewManager::splitView ( Qt::Orientation orientation, 
			       BrowserView *newView,
			       const QStringList &newViewServiceTypes )
{
  kdebug(0, 1202, "KonqViewManager::splitview" );
  
  if( m_pMainContainer )
  {
    KonqChildView* currentChildView = m_pMainView->currentChildView();
    KonqFrame* viewFrame = currentChildView->frame();
    KonqFrameContainer* parentContainer = viewFrame->parentContainer();
    bool moveNewContainer = (parentContainer->idAfter( viewFrame ) != 0);

    if( moveNewContainer )       
      kdebug(0, 1202, "Move new splitter: Yes %d",parentContainer->idAfter( viewFrame ) );
    else
      kdebug(0, 1202, "Move new splitter: No %d",parentContainer->idAfter( viewFrame ) );
    
    QValueList<int> sizes;
    sizes = parentContainer->sizes();

    QPoint pos = viewFrame->pos();

    viewFrame->hide();
    viewFrame->reparent( m_pMainView, 0, pos );
    
    KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer );
    if( moveNewContainer )
      parentContainer->moveToFirst( newContainer );

    viewFrame->reparent( newContainer, 0, pos, true );

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
    m_pMainContainer->setGeometry( 0, 0, m_pMainView->width(), m_pMainView->height() );

    setupView( m_pMainContainer, newView, newViewServiceTypes );

    // exclude the splitter and all child widgets from the part focus handling
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

  m_pMainView->removeChildView( view );

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
//    cfg.setGroup(  m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 ) );
    QString prefix = m_pMainContainer->firstChild()->frameType() + QString("%1").arg( 0 );
    prefix.append( '_' );
    m_pMainContainer->firstChild()->saveConfig( &cfg, prefix, 0, 1 );
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
  
  QValueList<BrowserView *> lst = m_pMainView->viewList();
  m_pMainView->setActiveView( (*lst.begin()) );
}

void KonqViewManager::loadItem( KConfig &cfg, KonqFrameContainer *parent, 
				const QString &name )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + '_';
//    cfg.setGroup( name );

  kdebug(0, 1202, "begin loadItem: %s",name.data() );  

  if( name.find("View") != -1 ) {
    kdebug(0, 1202, "Item is View");  
    //load view config
    QString url = cfg.readEntry( QString::fromLatin1( "URL" ).prepend( prefix ), QDir::homeDirPath() );
    kdebug(0, 1202, "URL: %s",url.data());  
    QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), "inode/directory");
    kdebug(0, 1202, "ServiceType: %s", serviceType.data());  

    Konqueror::DirectoryDisplayMode dirMode = Konqueror::LargeIcons;

    if ( serviceType == "inode/directory" )
    {
      QString strDirMode = cfg.readEntry( QString::fromLatin1( "DirectoryMode" ).prepend( prefix ), "LargeIcons" );

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

    BrowserView *pView;
    QStringList serviceTypes;

    //Simon TODO: error handling
    pView = KonqFactory::createView( serviceType, serviceTypes, dirMode);
    if( pView ) {
      kdebug(0, 1202, "Creating View Stuff");  
      setupView( parent, pView, serviceTypes);

      m_pMainView->childView( pView )->openURL( url );
    }
    else
      warning("Profile Loading Error: View creation failed" );
  }
  else if( name.find("Container") != -1 ) {
    kdebug(0, 1202, "Item is Container");  

    //load container config
    QString ostr = cfg.readEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ) );
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
      QVariant( cfg.readPropertyEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), 
				       QVariant::IntList ) ).intListValue();

    QStrList childList;
    if( cfg.readListEntry( QString::fromLatin1( "Children" ).prepend( prefix ), childList ) < 2 )
    {
      warning("Profile Loading Error: Less than two children in %s", name.data());
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
    warning("Profile Loading Error: Unknown item %s", name.data());

  kdebug(0, 1202, "end loadItem: %s",name.data());  
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

void KonqViewManager::doGeometry( int width, int height )
{
  if ( m_pMainContainer )
    m_pMainContainer->setGeometry( 0, 0, width, height );
}

KonqChildView *KonqViewManager::chooseNextView( KonqChildView *view )
{
  QList<KonqChildView> viewList;

  m_pMainContainer->listViews( &viewList );

  if ( !view )
    return viewList.first();

  int pos = viewList.find( view ) + 1;
  if( pos >= (int)viewList.count() )
    pos = 0;
  kdebug(0, 1202, "next view: %d", pos );  

  return viewList.at( pos );
}
#warning FIXME (Simon)
/*
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
*/

bool KonqViewManager::eventFilter( QObject *obj, QEvent *ev )
{
  if ( ev->type() == QEvent::MouseButtonPress ||
       ev->type() == QEvent::MouseButtonDblClick )
  {
    if ( !obj->isWidgetType() )
      return false;
      
    QWidget *w = (QWidget *)obj;
    
    if ( ( w->testWFlags( WStyle_Dialog ) && w->isModal() ) ||
           w->testWFlags( WType_Popup ) )
      return false;
    
    while ( w )
    {
      
      // check for the correct type and see if we "own" the view
      if ( w->inherits( "BrowserView" ) && m_pMainView->childView( (BrowserView *)w ) &&
           ((BrowserView *)w) != m_pMainView->currentView() )
      {
        m_pMainView->setActiveView( (BrowserView *)w );
        return false;
      }
      
      w = w->parentWidget();

      if ( w && ( ( w->testWFlags( WStyle_Dialog ) && w->isModal() ) ||
                  w->testWFlags( WType_Popup ) ) )
          return false;

    }
    
  }

  return false;
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

void KonqViewManager::setupView( KonqFrameContainer *parentContainer, BrowserView *view, const QStringList &serviceTypes )
{
  KonqFrame* newViewFrame = new KonqFrame( parentContainer );

  KonqChildView *v = new KonqChildView( view, newViewFrame, 
					m_pMainView, serviceTypes );

  QObject::connect( v, SIGNAL( sigViewChanged( BrowserView *, BrowserView * ) ), 
                    m_pMainView, SLOT( slotViewChanged( BrowserView *, BrowserView * ) ) );

  m_pMainView->insertChildView( v );

  v->lockHistory();

  //if (isVisible()) v->show();
}

void KonqViewManager::slotProfileActivated( int id )
{
  QString name = QString::fromLatin1( "konqueror/profiles/" ) + m_pamProfiles->popupMenu()->text( id );
  
  QString fileName = locate( "data", name, KonqFactory::instance() );
  
  KConfig cfg( fileName, true );
  cfg.setGroup( "Profile" );
  loadViewProfile( cfg );
}

void KonqViewManager::slotProfileListAboutToShow()
{
  if ( !m_pamProfiles || !m_bProfileListDirty )
    return;
    
  QPopupMenu *popup = m_pamProfiles->popupMenu();
  
  popup->clear();
  
  QStringList dirs = KonqFactory::instance()->dirs()->findDirs( "data", "konqueror/profiles/" );
  QStringList::ConstIterator dIt = dirs.begin();
  QStringList::ConstIterator dEnd = dirs.end();
  
  for (; dIt != dEnd; ++dIt )
  {
    QDir dir( *dIt );
    QStringList entries = dir.entryList( QDir::Files );
    
    QStringList::ConstIterator eIt = entries.begin();
    QStringList::ConstIterator eEnd = entries.end();
    
    for (; eIt != eEnd; ++eIt )
      popup->insertItem( *eIt );
  }
  
  m_bProfileListDirty = false;
}

#include "konq_viewmgr.moc"
