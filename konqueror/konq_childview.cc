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
 The award for the worst hack (aka Bug Oscar) goes to..... ah no, 
 it's not Microsoft, it's me :-(
 Well, there's somewhere in there a bug in regard to freeing references
 of a view. As a quick/bad hack we "force" the destruction of the object 
 (in case of a local view, for a remote view this doesn't matter) by
 simply "deref'ing" .... (ask me for further details about this)
 In contrary to the Microsoft way of hacking this hack should increase the
 stability of the product ;-) . Anyway, the problem remains and this is not
 meant to be a solution.
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
  m_strLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_vParent = OpenParts::Part::_duplicate( parent );
  m_vMainWindow = OpenParts::MainWindow::_duplicate( mainWindow );

  if (newViewPosition == Konqueror::left)
    m_row->moveToFirst( m_pFrame );
  
  attach( view );
  m_vView->show( true );
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
  assert(m_pFrame);
  m_pFrame->repaint();
}

void KonqChildView::openURL( QString url )
{
  emit sigSetUpEnabled( url, m_vView->id() );
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
  debug("switchView : part->inactive");
  m_vMainWindow->setActivePart( m_vParent->id() );
  debug("switchView : oldId=");
  OpenParts::Id oldId = m_vView->id();
    
  detach();
  Konqueror::View_var vView = Konqueror::View::_duplicate( _vView );
  debug("switchView : attaching new one");
  attach( vView );
    
  debug("switchView : emitting sigIdChanged");
  emit sigIdChanged( this, oldId, vView->id() );
  debug("switchView : setActivePart");
  m_vMainWindow->setActivePart( vView->id() ); 
}

void KonqChildView::changeViewMode( const char *viewName )
{
  QString vn = m_vView->viewName();
  cerr << "current view is a " << vn << endl;
  QString sViewURL = m_vView->url();
  
  // check the current view name against the asked one
  if ( strcmp( viewName, vn ) != 0L )
  {
    Konqueror::View_var vView = createViewByName( viewName ); 
    switchView( vView );
    openURL( sViewURL );
  }
}

Konqueror::View_ptr KonqChildView::createViewByName( const char *viewName )
{
  Konqueror::View_var vView;

  cerr << "void KonqChildView::createViewByName( " << viewName << " )" << endl;

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
    
    assert( KonqPlugins::isPluginServiceType( serviceType ) );
    
    CORBA::Object_var obj = KonqPlugins::lookupServer( serviceType, KonqPlugins::View );
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
    cerr << "WARNING: view does not know signal ""openURL"" " << endl;
  }
  try
  {
    m_vView->connect("started", m_vParent, "slotURLStarted");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""started"" " << endl;
  }
  try
  {
    m_vView->connect("completed", m_vParent, "slotURLCompleted");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""completed"" " << endl;
  }
  try
  {
    m_vView->connect("setStatusBarText", m_vParent, "setStatusBarText");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setStatusBarText"" " << endl;
  }
  try
  {
    m_vView->connect("setLocationBarURL", m_vParent, "setLocationBarURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setLocationBarURL"" " << endl;
  }
  try
  {
    m_vView->connect("createNewWindow", m_vParent, "createNewWindow");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""createNewWindow"" " << endl;
  }
  try
  {
    m_vView->connect("popupMenu", m_vParent, "popupMenu");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""popupMenu"" " << endl;
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
    h.strURL = m_strLastURL; // use url from last call
    h.strViewName = m_vView->viewName();
    
    m_tmpInternalHistoryEntry = h;
    m_strLastURL = url; // remember for next call
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
  cerr << "restoring " << h.entry.url << endl;      

  changeViewMode( h.strViewName );

  if ( h.bHasHistoryEntry )  
    m_vView->restoreState( h.entry );
  else
    openURL( h.strURL );
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
}

#include "konq_childview.moc"
