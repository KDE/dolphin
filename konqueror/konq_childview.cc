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
#include "konq_mainview.h"
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
                              OpenParts::MainWindow_ptr mainWindow,
                              KonqMainView * mainView
                              )
{
  m_pFrame = new KonqFrame( row );
  m_bCompleted = false;
  m_row = row;
  m_strLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_iHistoryLock = 0;
  m_vParent = OpenParts::Part::_duplicate( parent );
  m_vMainWindow = OpenParts::MainWindow::_duplicate( mainWindow );
  m_mainView = mainView;

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
  m_pFrame->detach();
  m_pFrame->hide();
  m_vView->decRef();
  VeryBadHackToFixCORBARefCntBug( m_vView );
  m_vView = 0L;
}

void KonqChildView::repaint()
{
  m_pFrame->repaint();
}

int KonqChildView::changeViewMode( const char *viewName )
{
  CORBA::String_var sViewURL = m_vView->url();

  detach();
  Konqueror::View_var vView = KonqChildView::createViewByName( viewName );
  attach( vView );
/*
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( sViewURL.in() );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( vView, Konqueror::eventOpenURL, eventURL );
*/  
  return vView->id();
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
    QString *serviceType = m_mainView->getServiceType( viewName );
    assert( !serviceType->isNull() );
    
    assert( KonqPlugins::isPluginServiceType( *serviceType ) );
    
    CORBA::Object_var obj = KonqPlugins::lookupServer( *serviceType, KonqPlugins::View );
    assert( !CORBA::is_nil( obj ) );
    
    Konqueror::ViewFactory_var factory = Konqueror::ViewFactory::_narrow( obj );
    assert( !CORBA::is_nil( obj ) );
    
    vView = Konqueror::View::_duplicate( factory->create() );
    assert( !CORBA::is_nil( vView ) );
  }
  
  return Konqueror::View::_duplicate( vView );
}

void KonqChildView::connectView(  )
{
  try
  {
    m_vView->connect("openURL", m_mainView, "openURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""openURL"" " << endl;
  }
  try
  {
    m_vView->connect("started", m_mainView, "slotURLStarted");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""started"" " << endl;
  }
  try
  {
    m_vView->connect("completed", m_mainView, "slotURLCompleted");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""completed"" " << endl;
  }
  try
  {
    m_vView->connect("setStatusBarText", m_mainView, "setStatusBarText");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setStatusBarText"" " << endl;
  }
  try
  {
    m_vView->connect("setLocationBarURL", m_mainView, "setLocationBarURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setLocationBarURL"" " << endl;
  }
  try
  {
    m_vView->connect("createNewWindow", m_mainView, "createNewWindow");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""createNewWindow"" " << endl;
  }
  try
  {
    m_vView->connect("popupMenu", m_mainView, "popupMenu");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""popupMenu"" " << endl;
  }

}

#include "konq_childview.moc"
