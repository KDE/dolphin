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
#include "konq_iconview.h"
#include "konq_treeview.h"
#include "konq_txtview.h"
#include "konq_htmlview.h"
#include "konq_plugins.h"
#include "konq_propsview.h"
#include "kfmrun.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <kactivator.h>

/*
 This is a _very_ bad hack to fix some buggy reference count handling somewhere
 in Konqy. We call this hack when we want to destroy a view, I mean really
 destroy. For remote views this bug is no real problem since the reference 
 counter on the server's side (the remote view) is not affected by our buggy 
 referencing. But for local views, where proxy/stub == server, this _is_ a 
 problem. So that's why we "kill" the CORBA reference counter this way, which
 helps us to make sure that the object will really die.
 Please don't misunderstand, this is meant to be a hack, not a solution or
 a simple workaround. We NEED/SHOULD fix the real bug instead ASAP.
 Simon
 */
void VeryBadHackToFixCORBARefCntBug( CORBA::Object_ptr obj )
{
  while ( obj->_refcnt() > 1 ) obj->_deref();
}

KonqChildView::KonqChildView( Browser::View_ptr view, 
			      KonqFrame* viewFrame,
			      KonqMainView *mainView,
			      const QStringList &serviceTypes
                              )
{
  m_pKonqFrame = viewFrame;

  m_sLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_pMainView = mainView;
  m_vMainWindow = mainView->mainWindow();
  m_pRun = 0L;
  m_pRow = 0L;

  attach( view );

  m_lstServiceTypes = serviceTypes;
  m_bAllowHTML = KonqPropsView::defaultProps()->isHTMLAllowed();
  m_lstBack.setAutoDelete( true );
  m_lstForward.setAutoDelete( true );
  m_bReloadURL = false;
  m_iXOffset = 0;
  m_iYOffset = 0;
}

KonqChildView::~KonqChildView()
{
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

  m_pKonqFrame->attach( view );
  m_pKonqFrame->show();

  KonqPlugins::installKOMPlugins( view );
}

void KonqChildView::detach()
{
  m_pKonqFrame->hide();
  m_pKonqFrame->detach();

  m_vView->disconnectObject( m_pMainView );
  m_vView->decRef(); //die view, die ... (cruel world, isn't it?) ;)
  VeryBadHackToFixCORBARefCntBug( m_vView );
  m_vView = 0L; //now it _IS_ dead
}

void KonqChildView::repaint()
{
  kdebug(0,1202,"KonqChildView::repaint()");
  if (m_pKonqFrame != 0L) 
    m_pKonqFrame->repaint();
  kdebug(0,1202,"KonqChildView::repaint() : done");
}

void KonqChildView::show()
{
  kdebug(0,1202,"KonqChildView::show()");
  m_vView->show( true );
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
                                    const QString &_url, bool useMiscURLData )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();
    
  //no need to change anything if we are able to display this servicetype
  if ( m_lstServiceTypes.find( serviceType ) != m_lstServiceTypes.end() )
  {
    makeHistory( false );
    openURL( url, useMiscURLData );
    return true;
  }

  Browser::View_var vView;
  QStringList serviceTypes;
  if ( !createView( serviceType, vView, serviceTypes, m_pMainView ) )
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
    
  OpenParts::Id oldId = m_vView->id();
  switchView( _vView, serviceTypes );
  
  emit sigIdChanged( this, oldId, _vView->id() );
  
  openURL( url );
  
  m_vMainWindow->setActivePart( _vView->id() );
}

void KonqChildView::connectView(  )
{
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

}

void KonqChildView::makeHistory( bool pushEntry )
{
  //HACK
  if ( !m_bHistoryLock && m_pCurrentHistoryEntry && pushEntry &&
       ( m_pCurrentHistoryEntry->strServiceType == "text/html" ) &&
       m_vView->supportsInterface( "IDL:Konqueror/HTMLView:1.0" ) )
  {
    m_pCurrentHistoryEntry->xOffset = m_vView->xOffset();
    m_pCurrentHistoryEntry->yOffset = m_vView->yOffset();
  }

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
         m_lstBack.append( m_pCurrentHistoryEntry );
      } 
      else 
      {
        m_lstForward.clear();
        kdebug(0,1202,"pushing into backward history : %s", m_pCurrentHistoryEntry->strURL.ascii() );
        m_lstBack.append( m_pCurrentHistoryEntry );
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
}

void KonqChildView::goBack()
{
  assert( m_lstBack.count() != 0 );

  HistoryEntry *h = m_lstBack.take( m_lstBack.count()-1 );
  m_bBack = true;

  m_bReloadURL = false;
  m_iXOffset = h->xOffset;
  m_iYOffset = h->yOffset;
  changeViewMode( h->strServiceType, h->strURL );
  delete h;
}

void KonqChildView::goForward()
{
  assert( m_lstForward.count() != 0 );

  HistoryEntry *h = m_lstForward.take( 0 );
  m_bForward = true;

  m_bReloadURL = false;
  m_iXOffset = h->xOffset;
  m_iYOffset = h->yOffset;
  changeViewMode( h->strServiceType, h->strURL );
  delete h;
}

QString KonqChildView::url()
{
  assert( m_vView );
  CORBA::String_var u = m_vView->url();
  QString url( u.in() );
  return url;
}

QString KonqChildView::viewName()
{
  CORBA::String_var v = m_vView->viewName();
  QString viewName( v.in() );
  return viewName;
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

bool KonqChildView::supportsServiceType( const QString &serviceType )
{
  bool result = m_lstServiceTypes.contains( serviceType );
  kdebug(0, 1202, "KonqChildView::supportsServiceType(%s) : returning %d", serviceType.data(), result);
  return result;
}

bool KonqChildView::createView( const QString &serviceType, 
                                Browser::View_var &view, 
				QStringList &serviceTypes,
				KonqMainView *mainView )
{
  serviceTypes.clear();

  kdebug(0,1202,"trying to create view for %s", serviceType.ascii());
  
  //check for builtin views first
  if ( serviceType == "inode/directory" )
  {
    //default for directories is the iconview
    view = Browser::View::_duplicate( new KonqKfmIconView( mainView ) );
    serviceTypes.append( serviceType );
    return true;
  }
  else if ( serviceType == "text/html" )
  {
    view = Browser::View::_duplicate( new KonqHTMLView( mainView ) );
    serviceTypes.append( serviceType );
    return true;
  }
  else if ( serviceType.left( 5 ) == "text/" &&
            ( serviceType.mid( 5, 2 ) == "x-" ||
	      serviceType.mid( 5 ) == "english" ||
	      serviceType.mid( 5 ) == "plain" ) )
  {
    view = Browser::View::_duplicate( new KonqTxtView( mainView ) );
    serviceTypes.append( serviceType );
    return true;
  }
  
  //now let's query the Trader for view plugins
  KTrader *trader = KdedInstance::self()->ktrader();
  KActivator *activator = KdedInstance::self()->kactivator();
  
  KTrader::OfferList offers = trader->query( serviceType, "'Browser/View' in ServiceTypes" );
  
  if ( offers.count() == 0 ) //no results?
    return false;
    
  //activate the view plugin
  KService::Ptr service = offers.first();
  
  if ( service->repoIds().count() == 0 )  //uh...is it a CORBA service at all??
    return false;
 
  QString repoId = service->repoIds().first();
  QString tag = service->name(); //use service name as default tag
  int tagPos = repoId.find( "#" );
  if ( tagPos != -1 )
  {
    tag = repoId.mid( tagPos + 1 );
    repoId.truncate( tagPos );
  }  

  CORBA::Object_var obj = activator->activateService( service->name(), repoId, tag );
  
  Browser::ViewFactory_var factory = Browser::ViewFactory::_narrow( obj );
  
  if ( CORBA::is_nil( factory ) )
    return false;

  kdebug(0, 1202, "KonqChildView: creating view!");
  view = factory->create();
  kdebug(0, 1202, "KonqChildView: creating view done");

  if ( CORBA::is_nil( view ) )
    return false;

  serviceTypes = service->serviceTypes();

  return true;
}

#include "konq_childview.moc"
