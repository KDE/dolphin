/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
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

#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_propsview.h"
#include "konq_factory.h"
#include "konq_run.h"

#include <assert.h>

#include <kdebug.h>

KonqChildView::KonqChildView( BrowserView *view, 
			      KonqFrame* viewFrame,
			      KonqMainView *mainView,
			      const QStringList &serviceTypes
                              )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setChildView( this );

  m_sLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_pMainView = mainView;
  m_pRun = 0L;

  attach( view );

  m_lstServiceTypes = serviceTypes;
  m_bAllowHTML = KonqPropsView::defaultProps()->isHTMLAllowed();
  m_lstBack.setAutoDelete( true );
  m_lstForward.setAutoDelete( true );
  m_bReloadURL = false;
  m_iXOffset = 0;
  m_iYOffset = 0;
  m_bLoading = false;
  m_iProgress = -1;
  m_bPassiveMode = false;
  m_bProgressSignals = true;
}

KonqChildView::~KonqChildView()
{
  kdebug(0,1202,"KonqChildView::~KonqChildView");
  detach();
  delete m_pKonqFrame;
  if ( m_pRun )
    delete (KonqRun *)m_pRun;
}

void KonqChildView::attach( BrowserView *view )
{
  m_pView = view;
  connectView( );

  m_pKonqFrame->show();
  m_pKonqFrame->attach( view );

//  KonqPlugins::installKOMPlugins( view );
}

void KonqChildView::detach()
{
//  m_pKonqFrame->hide();
  m_pKonqFrame->detach();
  
  QObject *obj = m_pView->child( 0L, "EditExtension" );
  if ( obj )
    obj->disconnect( m_pMainView );

/*
  if ( m_vView->supportsInterface( "IDL:Browser/EditExtension:1.0" ) )
  {
    CORBA::Object_var obj = m_vView->getInterface( "IDL:Browser/EditExtension:1.0" );
    Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
    editExtension->disconnectObject( m_pMainView );
  }
*/
  
  m_pView->disconnect( m_pMainView );
  delete m_pView;
  m_pView = 0L;
}

void KonqChildView::repaint()
{
//  kdebug(0,1202,"KonqChildView::repaint()");
  if (m_pKonqFrame != 0L) 
    m_pKonqFrame->repaint();
//  kdebug(0,1202,"KonqChildView::repaint() : done");
}

void KonqChildView::show()
{
  kdebug(0,1202,"KonqChildView::show()");
//  m_vView->show( true );
  if ( m_pKonqFrame ) 
    m_pKonqFrame->show();
}

void KonqChildView::openURL( const QString &url, bool useMiscURLData  )
{
  if ( useMiscURLData )
    m_pView->openURL( url, m_bReloadURL, m_iXOffset, m_iYOffset );
  else
    m_pView->openURL( url );

  m_pMainView->setLocationBarURL( this, url );
}

void KonqChildView::switchView( BrowserView *pView, const QStringList &serviceTypes )
{
  detach();
  attach( pView );

  m_lstServiceTypes = serviceTypes;
  
  show(); // switchView is never called on startup. We can always show() the view.
}

bool KonqChildView::changeViewMode( const QString &serviceType, 
                                    const QString &_url, bool useMiscURLData,
				    Konqueror::DirectoryDisplayMode dirMode )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();

  if ( m_bLoading )
    stop();

  //no need to change anything if we are able to display this servicetype
  if ( m_lstServiceTypes.find( serviceType ) != m_lstServiceTypes.end() &&
       serviceType != "inode/directory" )
  {
    makeHistory( false );
    openURL( url, useMiscURLData );
    return true;
  }

  BrowserView *pView;
  QStringList serviceTypes;
  if ( !( pView = KonqFactory::createView( serviceType, serviceTypes, dirMode ) ) )
   return false;
  makeHistory( false );
  BrowserView *oldView = m_pView;
  
  emit sigViewChanged( oldView, pView );
  
  switchView( pView, serviceTypes );
  
  openURL( url, useMiscURLData );
  
  return true;
}

void KonqChildView::changeView( BrowserView *pView, 
                                const QStringList &serviceTypes, 
				const QString &_url )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();
    
  BrowserView *oldView = m_pView;

  emit sigViewChanged( oldView, pView );
  
  switchView( pView, serviceTypes );
  
  
  openURL( url );
}

void KonqChildView::connectView(  )
{

  connect( m_pView, SIGNAL( openURLRequest( const QString &, bool, int, int ) ),
           m_pMainView, SLOT( openURL( const QString &, bool, int, int ) ) );

  connect( m_pView, SIGNAL( started() ),
           m_pMainView, SLOT( slotStarted() ) );
  connect( m_pView, SIGNAL( completed() ),
           m_pMainView, SLOT( slotCompleted() ) );
  connect( m_pView, SIGNAL( canceled() ),
           m_pMainView, SLOT( slotCanceled() ) );

  connect( m_pView, SIGNAL( setStatusBarText( const QString & ) ),
           m_pMainView, SLOT( slotSetStatusBarText( const QString & ) ) );

  connect( m_pView, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
           m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

  QObject *obj = m_pView->child( 0L, "EditExtension" );
  if ( obj )
  {
    EditExtension *editExtension = (EditExtension *)obj;
    connect( editExtension, SIGNAL( selectionChanged() ),
             m_pMainView, SLOT( checkEditExtension() ) );
  }

  connect( m_pView, SIGNAL( setLocationBarURL( const QString & ) ),
           m_pMainView, SLOT( slotSetLocationBarURL( const QString & ) ) );

//  connect( m_pView, SIGNAL( createNewWindow( const QString & ) ),
//           m_pMainView, SLOT( slotCreateNewWindow( const QString ) ) );

  connect( m_pView, SIGNAL( loadingProgress( int ) ),
           m_pMainView, SLOT( slotLoadingProgress( int ) ) );

  connect( m_pView, SIGNAL( speedProgress( int ) ),
           m_pMainView, SLOT( slotSpeedProgress( int ) ) );

/*
  try
  {
    m_vView->connect("openURL", m_pMainView, "openURL");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""openURL"" ");
  }
  try
  {
    m_vView->connect("started", m_pMainView, "slotURLStarted");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""started"" ");
  }
  try
  {
    m_vView->connect("completed", m_pMainView, "slotURLCompleted");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""completed"" ");
  }
  try
  {
    m_vView->connect("setStatusBarText", m_pMainView, "setStatusBarText");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""setStatusBarText"" ");
  }
  try
  {
    m_vView->connect("setLocationBarURL", m_pMainView, "setLocationBarURL");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""setLocationBarURL"" ");
  }
  try
  {
    m_vView->connect("createNewWindow", m_pMainView, "createNewWindow");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""createNewWindow"" ");
  }
  try
  {
    m_vView->connect("loadingProgress", m_pMainView, "slotLoadingProgress");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""loadingProgress"" ");
    m_bProgressSignals = false;
  }
  try
  {
    m_vView->connect("speedProgress", m_pMainView, "slotSpeedProgress");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""speedProgress"" ");
    m_bProgressSignals = false;
  }

  if ( m_vView->supportsInterface( "IDL:Browser/EditExtension:1.0" ) )
  {
    CORBA::Object_var obj = m_vView->getInterface( "IDL:Browser/EditExtension:1.0" );
    Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
    try
    {
      editExtension->connect( "selectionChanged", m_pMainView, "slotSelectionChanged" );
    }
    catch ( ... )
    {
      kdebug(KDEBUG_WARN,1202,"WARNING: edit extension does not know signal ""selectionChanged"" ");
    }
    
  }
*/
}

void KonqChildView::makeHistory( bool pushEntry )
{
  if ( pushEntry )
  {
    if ( !m_bHistoryLock )
    {
      if ( m_bBack ) 
      {
        m_bBack = false;
        kdebug(0,1202,"pushing into forward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstForward.insert( 0, m_pCurrentHistoryEntry );
      }
      else if ( m_bForward ) 
      {
        m_bForward = false;
        kdebug(0,1202,"pushing into backward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      } 
      else 
      {
        m_lstForward.clear();
        kdebug(0,1202,"pushing into backward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      }
    }
    else
      m_bHistoryLock = false;
  }
  
  if ( pushEntry || !m_pCurrentHistoryEntry )
    m_pCurrentHistoryEntry = new HistoryEntry;

  m_pCurrentHistoryEntry->strURL = m_pView->url();
  m_pCurrentHistoryEntry->xOffset = m_pView->xOffset();
  m_pCurrentHistoryEntry->yOffset = m_pView->yOffset();
  m_pCurrentHistoryEntry->strServiceType = m_lstServiceTypes.first();
  
  if ( m_pCurrentHistoryEntry->strServiceType == "inode/directory" )
  {
//    if ( m_vView->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
#warning FIXME (Simon)
  m_pCurrentHistoryEntry->eDirMode = Konqueror::LargeIcons;
//    if ( m_pView->inherits( "KfmTreeView" ) )
//      m_pCurrentHistoryEntry->eDirMode = Konqueror::TreeView;
//    else
//    {
//      Konqueror::KfmIconView_var iconView = Konqueror::KfmIconView::_narrow( m_vView );
//      KonqKfmIconView *iconView = (KonqKfmIconView *)m_pView;
//      
//      m_pCurrentHistoryEntry->eDirMode = iconView->KonqKfmIconView::viewMode();
//    }
  }
}

void KonqChildView::go( QList<HistoryEntry> &stack, int steps )
{
  assert( (int)stack.count() >= steps );
  
  for ( int i = 0; i < steps-1; i++ )
    stack.removeFirst();

  HistoryEntry *h = stack.first();
  
  assert( h );
  
  m_bReloadURL = false;
  m_iXOffset = h->xOffset;
  m_iYOffset = h->yOffset;
  changeViewMode( h->strServiceType, h->strURL, true, h->eDirMode );
  
  stack.removeFirst();
}

void KonqChildView::goBack( int steps )
{
  m_bBack = true;
  go( m_lstBack, steps );
}

void KonqChildView::goForward( int steps )
{
  m_bForward = true;
  go( m_lstForward, steps );
}

QStringList KonqChildView::backHistoryURLs()
{
  QStringList res;
  
  QListIterator<HistoryEntry> it( m_lstBack );
  for (; it.current(); ++it )
    res.append( it.current()->strURL );
    
  return res;
}

QStringList KonqChildView::forwardHistoryURLs()
{
  QStringList res;
  
  QListIterator<HistoryEntry> it( m_lstForward );
  for (; it.current(); ++it )
    res.append( it.current()->strURL );
  
  return res;
}

QString KonqChildView::url()
{
  assert( m_pView );
  return m_pView->url();
}

void KonqChildView::run( const QString & url )
{
  debug(" ********** KonqChildView::run ");
  m_pRun = new KonqRun( mainView(), this, url, 0, false, true );
  // stop() will get called by KonqMainView::openView or completely destroyed
}

void KonqChildView::stop()
{
  m_pView->stop();

  if ( m_pRun ) debug(" m_pRun is not NULL ");
  else debug(" m_pRun is NULL ");
//  if ( m_pRun ) delete (KonqRun *)m_pRun; // HACK !!!!!
  m_pRun = 0L;
}

void KonqChildView::reload()
{
  m_bForward = false;
  m_bBack = false;
  lockHistory();
  
  m_pView->openURL( m_pView->url(), true, m_pView->xOffset(), m_pView->yOffset() );
}

#include "konq_childview.moc"
