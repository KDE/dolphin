/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_partview.h"
#include "konq_iconview.h"
#include "konq_treeview.h"
#include "konq_txtview.h"
#include "konq_htmlview.h"
#include "konq_plugins.h"

#include <qsplitter.h>

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

KonqChildView::KonqChildView( Konqueror::View_ptr view, 
                              Row * row, 
                              Konqueror::NewViewPosition newViewPosition,
                              OpenParts::Part_ptr parent,
                              OpenParts::MainWindow_ptr mainWindow
                              )
{
  m_pFrame = new KonqFrame( row );
  m_row = row;
  m_sLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_vParent = OpenParts::Part::_duplicate( parent );
  m_vMainWindow = OpenParts::MainWindow::_duplicate( mainWindow );

  if (newViewPosition == Konqueror::left)
    m_row->moveToFirst( m_pFrame );
  
  attach( view );
  m_vView->show( true );
  m_sLastViewName = viewName();
}

KonqChildView::~KonqChildView()
{
  detach();
  delete m_pFrame;
}

void KonqChildView::attach( Konqueror::View_ptr view )
{
  m_vView = Konqueror::View::_duplicate( view );
  m_vView->setMainWindow( m_vMainWindow );
  m_vView->setParent( m_vParent );
  connectView( );
  m_pFrame->attach( view );
  m_pFrame->show();
  KonqPlugins::installKOMPlugins( view );
}

void KonqChildView::detach()
{
  m_pFrame->hide();
  m_pFrame->detach();
  m_vView->disconnectObject( m_vParent );
  m_vView->decRef(); //die view, die ... (cruel world, isn't it?) ;)
  VeryBadHackToFixCORBARefCntBug( m_vView );
  m_vView = 0L; //now it _IS_ dead
}

void KonqChildView::repaint()
{
  kdebug(0,1202,"KonqChildView::repaint()");
  assert(m_pFrame);
  m_pFrame->repaint();
}

void KonqChildView::openURL( QString url )
{
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( url.data() );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( m_vView, Konqueror::eventOpenURL, eventURL );
}

void KonqChildView::emitEventViewMenu( OpenPartsUI::Menu_ptr menu, bool create )
{
  Konqueror::View::EventCreateViewMenu EventViewMenu;
  EventViewMenu.menu = OpenPartsUI::Menu::_duplicate( menu );
  EventViewMenu.create = create;
  EMIT_EVENT( m_vView, Konqueror::View::eventCreateViewMenu, EventViewMenu );
}

void KonqChildView::switchView( Konqueror::View_ptr _vView )
{
  kdebug(0,1202,"switchView : part->inactive");
  m_vMainWindow->setActivePart( m_vParent->id() );
  kdebug(0,1202,"switchView : oldId=");
  OpenParts::Id oldId = m_vView->id();
    
  detach();
  Konqueror::View_var vView = Konqueror::View::_duplicate( _vView );
  kdebug(0,1202,"switchView : attaching new one");
  attach( vView );
    
  kdebug(0,1202,"switchView : emitting sigIdChanged");
  emit sigIdChanged( this, oldId, vView->id() );
  kdebug(0,1202,"switchView : setActivePart");
  m_vMainWindow->setActivePart( vView->id() ); 
}

void KonqChildView::changeViewMode( const char *viewName )
{
  // check the current view name against the asked one
  if ( strcmp( viewName, this->viewName() ) != 0L )
  {
    QString sViewURL = url(); // store current URL
    Konqueror::View_var vView = createViewByName( viewName ); 
    switchView( vView );
    openURL( sViewURL );
  }
}

Konqueror::View_ptr KonqChildView::createViewByName( const char *viewName )
{
  Konqueror::View_var vView;

  kdebug(0,1202,"void KonqChildView::createViewByName( %s )", viewName);

  //check for builtin views
  if ( strcmp( viewName, "KonquerorKfmIconView" ) == 0 )
  {
    vView = Konqueror::View::_duplicate( new KonqKfmIconView );
  }
  else if ( strcmp( viewName, "KonquerorKfmTreeView" ) == 0 )
  {
    vView = Konqueror::View::_duplicate( new KonqKfmTreeView );
  }
  else if ( strcmp( viewName, "KonquerorHTMLView" ) == 0 )
  {
    vView = Konqueror::View::_duplicate( new KonqHTMLView );
  }
  else if ( strcmp( viewName, "KonquerorPartView" ) == 0 )
  {
    vView = Konqueror::View::_duplicate( new KonqPartView );
  }
  else if ( strcmp( viewName, "KonquerorTxtView" ) == 0 )
  {
    vView = Konqueror::View::_duplicate( new KonqTxtView );
  }
  else
  {
    QString serviceType = KonqPlugins::getServiceType( viewName );
    assert( !serviceType.isNull() );
    
    CORBA::Object_var obj = KonqPlugins::lookupViewServer( serviceType );
    assert( !CORBA::is_nil( obj ) );
    
    Konqueror::ViewFactory_var factory = Konqueror::ViewFactory::_narrow( obj );
    assert( !CORBA::is_nil( obj ) );
    
    vView = Konqueror::View::_duplicate( factory->create() );
    assert( !CORBA::is_nil( vView ) );
  }

  vView->incRef(); //it's a little bit...uhm... tricky do increase the KOM
                   //reference counter here, because IMO it would make more
		   //sense in the constructor and in attach() .
		   //Nevertheless it works fine this way and it helps us to
		   //make sure that we're also "owner" of the view.
    
  return Konqueror::View::_duplicate( vView );
}

void KonqChildView::connectView(  )
{
  try
  {
    m_vView->connect("openURL", m_vParent, "openURL");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""openURL"" ");
  }
  try
  {
    m_vView->connect("started", m_vParent, "slotURLStarted");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""started"" ");
  }
  try
  {
    m_vView->connect("completed", m_vParent, "slotURLCompleted");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""completed"" ");
  }
  try
  {
    m_vView->connect("setStatusBarText", m_vParent, "setStatusBarText");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""setStatusBarText"" ");
  }
  try
  {
    m_vView->connect("setLocationBarURL", m_vParent, "setLocationBarURL");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""setLocationBarURL"" ");
  }
  try
  {
    m_vView->connect("createNewWindow", m_vParent, "createNewWindow");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""createNewWindow"" ");
  }
  try
  {
    m_vView->connect("popupMenu", m_vParent, "popupMenu");
  }
  catch ( ... )
  {
    kdebug(KDEBUG_WARN,1202,"WARNING: view does not know signal ""popupMenu"" ");
  }

}

void KonqChildView::makeHistory( bool bCompleted, QString url )
{
  InternalHistoryEntry h;

  if ( !bCompleted )
  {
    if ( !m_bHistoryLock ) // no lock
    {
      if ( m_bBack )
      {
        m_bBack = false;
        m_lstForward.push_front( m_tmpInternalHistoryEntry );
      }
      else if ( m_bForward )
      {
        m_bForward = false;
        m_lstBack.push_back( m_tmpInternalHistoryEntry );
      }
      else
      {
        m_lstForward.clear();
        m_lstBack.push_back( m_tmpInternalHistoryEntry );
      }	
    }
    else
      m_bHistoryLock = false;
  
    h.bHasHistoryEntry = false;
    h.strURL = m_sLastURL; // use url from last call
    h.strViewName = m_sLastViewName;
    
    m_tmpInternalHistoryEntry = h;
    m_sLastURL = url; // remember for next call
    m_sLastViewName = viewName();
  }
  else
  {
    h = m_tmpInternalHistoryEntry;
      
    h.bHasHistoryEntry = true;
    Konqueror::View::HistoryEntry_var state = m_vView->saveState();
    h.entry = state;

    m_tmpInternalHistoryEntry = h;
  }
}

void KonqChildView::goBack()
{
  assert( m_lstBack.size() != 0 );

  InternalHistoryEntry h = m_lstBack.back();
  m_lstBack.pop_back();
  m_bBack = true;
  kdebug(0,1202,"restoring %s with mode %s", h.entry.url.in(), h.strViewName.data());

  changeViewMode( h.strViewName );

  if ( h.bHasHistoryEntry )  
    m_vView->restoreState( h.entry );
  else
  {
    kdebug(0,1202,"restoring %s",h.strURL.data());
    openURL( h.strURL );
  }
}

void KonqChildView::goForward()
{
  assert( m_lstForward.size() != 0 );

  InternalHistoryEntry h = m_lstForward.front();
  m_lstForward.pop_front();
  m_bForward = true;

  changeViewMode( h.strViewName );
    
  if ( h.bHasHistoryEntry )  
    m_vView->restoreState( h.entry );
  else
    openURL( h.strURL );
}

QString KonqChildView::url()
{
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
  
// TODO (trivial)
// hm...perhaps I was wrong ;)
// I'll do it now like this:
  Konqueror::EventOpenURL eventURL;
  eventURL.url = m_vView->url();
  eventURL.reload = (CORBA::Boolean)true;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( m_vView, Konqueror::eventOpenURL, eventURL );
//but perhaps this would be better:
//(1) remove the reload/xOffset/yOffset stuff out of the event structure
//(2) add general methods like reload(), moveTo( xofs, yofs) to the view interface
// What do you think, David?
//(Simon)

  // Hmm, if we need the current structure, we need to be able to _get_ the 
  // xofs and yofs, in order to fill the above. Can we do that ? (David)
  
  // very true... We should fix this ASAP (the event structure as well as the
  // history functionality, bound to it in somehow (--> for example when pressing
  // the back button I want the previous view to be at the previous x/y offset
  // Hm....
  // (Simon)
}

#include "konq_childview.moc"
