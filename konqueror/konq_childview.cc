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
#include "konq_plugins.h"
#include "konq_propsview.h"
#include "konq_factory.h"
#include "kfmrun.h"

KonqChildView::KonqChildView( Browser::View_ptr view, 
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
  m_vMainWindow = mainView->mainWindow();
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
  m_iProgress = 0;
  m_bPassiveMode = false;
  m_bProgressSignals = false;
}

KonqChildView::~KonqChildView()
{
  kdebug(0,1202,"KonqChildView::~KonqChildView");
  detach();
  delete m_pKonqFrame;
  if ( m_pRun )
    delete m_pRun;
}

void KonqChildView::attach( Browser::View_ptr view )
{
  m_vView = Browser::View::_duplicate( view );
  m_vView->incRef();
  m_vView->setMainWindow( m_vMainWindow );
  m_vView->setParent( m_pMainView );
  connectView( );

  m_pKonqFrame->show();
  m_pKonqFrame->attach( view );

  KonqPlugins::installKOMPlugins( view );
}

void KonqChildView::detach()
{
//  m_pKonqFrame->hide();
  m_pKonqFrame->detach();

  if ( m_vView->supportsInterface( "IDL:Browser/EditExtension:1.0" ) )
  {
    CORBA::Object_var obj = m_vView->getInterface( "IDL:Browser/EditExtension:1.0" );
    Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
    editExtension->disconnectObject( m_pMainView );
  }
  
  m_vView->disconnectObject( m_pMainView );
  m_vView->decRef(); //die view, die ... (cruel world, isn't it?) ;)
  m_vView = 0L; //now it _IS_ dead
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
  Browser::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( url.data() );
  if ( useMiscURLData )
  {
    eventURL.reload = (CORBA::Boolean)m_bReloadURL;
    eventURL.xOffset = m_iXOffset;
    eventURL.yOffset = m_iYOffset;
  }
  else
  {
    eventURL.reload = (CORBA::Boolean)false;
    eventURL.xOffset = 0;
    eventURL.yOffset = 0;
  }    
  EMIT_EVENT( m_vView, Browser::eventOpenURL, eventURL );
}

void KonqChildView::switchView( Browser::View_ptr _vView, const QStringList &serviceTypes )
{
  kdebug(0,1202,"switchView : part->inactive");
  m_vMainWindow->setActivePart( m_pMainView->id() );
    
  detach();
  Browser::View_var vView = Browser::View::_duplicate( _vView );
  kdebug(0,1202,"switchView : attaching new one");
  attach( vView );

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

  Browser::View_var vView;
  QStringList serviceTypes;
  if ( CORBA::is_nil( ( vView = KonqFactory::createView( serviceType, serviceTypes, m_pMainView, dirMode ) ) ) )
   return false;
  makeHistory( false );
  OpenParts::Id oldId = m_vView->id();
  switchView( vView, serviceTypes );
  
  emit sigIdChanged( this, oldId, vView->id() );
  
  openURL( url, useMiscURLData );
  
  m_vMainWindow->setActivePart( vView->id() ); 
  return true;
}

void KonqChildView::changeView( Browser::View_ptr _vView, 
                                const QStringList &serviceTypes, 
				const QString &_url )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();
    
  m_vMainWindow->setActivePart( m_pMainView->id() );
    
  OpenParts::Id oldId = m_vView->id();
  switchView( _vView, serviceTypes );
  
  emit sigIdChanged( this, oldId, _vView->id() );
  
  openURL( url );
  
  m_vMainWindow->setActivePart( _vView->id() );
}

void KonqChildView::connectView(  )
{
  m_bProgressSignals = true;
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

  CORBA::String_var curl = m_vView->url();
  m_pCurrentHistoryEntry->strURL = QString( curl.in() );
  m_pCurrentHistoryEntry->xOffset = m_vView->xOffset();
  m_pCurrentHistoryEntry->yOffset = m_vView->yOffset();
  m_pCurrentHistoryEntry->strServiceType = m_lstServiceTypes.first();
  
  if ( m_pCurrentHistoryEntry->strServiceType == "inode/directory" )
  {
    if ( m_vView->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
      m_pCurrentHistoryEntry->eDirMode = Konqueror::TreeView;
    else
    {
      Konqueror::KfmIconView_var iconView = Konqueror::KfmIconView::_narrow( m_vView );
      
      m_pCurrentHistoryEntry->eDirMode = iconView->viewMode();
    }
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
  assert( m_vView );
  CORBA::String_var u = m_vView->url();
  QString url( u.in() );
  return url;
}

void KonqChildView::reload()
{
  m_bForward = false;
  m_bBack = false;
  lockHistory();
  
  Browser::EventOpenURL eventURL;
  eventURL.url = m_vView->url();
  eventURL.reload = (CORBA::Boolean)true;
  eventURL.xOffset = m_vView->xOffset();
  eventURL.yOffset = m_vView->yOffset();
  EMIT_EVENT( m_vView, Browser::eventOpenURL, eventURL );
}

#include "konq_childview.moc"
