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
#include "konq_partview.h"
#include "konq_iconview.h"
#include "konq_treeview.h"
#include "konq_txtview.h"
#include "konq_htmlview.h"
#include "konq_plugins.h"
#include "konq_propsview.h"

#include <kded_instance.h>
#include <ktrader.h>
#include <kactivator.h>

#include <opFrame.h>
#include <qsplitter.h>
#include <qlayout.h>

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

// Let's do without it while it's broken ...
// #define USE_QXEMBED

KonqChildView::KonqChildView( Konqueror::View_ptr view, 
                              Row * row, 
                              NewViewPosition newViewPosition,
                              OpenParts::Part_ptr parent,
                              QWidget * ,
                              OpenParts::MainWindow_ptr mainWindow,
			      const QStringList &serviceTypes
                              )
  : m_row( row )
{
  // Hmmm, we need to add a QWidget in the splitter, not a simple layout
  // (moveToFirst needs a widget, and a layout can't be a splitter child I think)
  m_pWidget = new QWidget( row );

  // add the frame header to the layout
  m_pHeader = new KonqFrameHeader( view, m_pWidget, "KonquerorFrameHeader");
  QObject::connect(m_pHeader, SIGNAL(headerClicked()), this, SLOT(slotHeaderClicked()));

  m_pFrame = 0L;
  m_pLayout = 0L;
  m_sLocationBarURL = "";
  m_bBack = false;
  m_bForward = false;
  m_bHistoryLock = false;
  m_vParent = OpenParts::Part::_duplicate( parent );
  m_vMainWindow = OpenParts::MainWindow::_duplicate( mainWindow );

  if (newViewPosition == left)
    m_row->moveToFirst( m_pWidget );
  
  attach( view );

  m_lstServiceTypes = serviceTypes;
  m_bAllowHTML = KonqPropsView::defaultProps()->isHTMLAllowed();
}

KonqChildView::~KonqChildView()
{
  detach();
  delete m_pWidget;
}

void KonqChildView::attach( Konqueror::View_ptr view )
{
  OPPartIf* localView = 0L;
  // Local or remote ? (Simon's trick ;)
  QListIterator<OPPartIf> it = OPPartIf::partIterator();
  for (; it.current(); ++it )
    if ( (*it)->window() == view->window() )
      localView = *it;

  m_pHeader->setPart( view );
  m_vView = Konqueror::View::_duplicate( view );
  m_vView->incRef();
  m_vView->setMainWindow( m_vMainWindow );
  m_vView->setParent( m_vParent );
  connectView( );
  if (m_pFrame) delete m_pFrame;
  if (m_pLayout) delete m_pLayout;

  m_pLayout = new QVBoxLayout( m_pWidget );
  m_pLayout->addWidget( m_pHeader );
#ifdef USE_QXEMBED
  m_pFrame = new OPFrame( m_pWidget );
  m_pLayout->addWidget( m_pFrame );
#endif
  if ( localView )
  {
    kdebug(0, 1202, " ************* LOCAL VIEW ! *************");
#ifdef USE_QXEMBED
    m_pFrame->attachLocal( localView );
#else 
    QWidget * localWidget = localView->widget();
    localWidget->reparent( m_pWidget, 0, QPoint(0, 0) );
    m_pLayout->addWidget( localWidget ); 
    m_pFrame = 0L;
#endif
  }
  else
  {
#ifndef USE_QXEMBED
    m_pFrame = new OPFrame( m_pWidget );
    m_pLayout->addWidget( m_pFrame );
#endif
    kdebug(0, 1202, " ************* NOT LOCAL :( *************");
    m_pFrame->attach( view );
  }

// disabled for now because it's slow (yet another trader query) and we
// don't have any KOM plugins for views, yet (Simon)
//  KonqPlugins::installKOMPlugins( view );
  m_pLayout->activate();
}

void KonqChildView::detach()
{
  if ( m_pFrame )
  {
    m_pFrame->hide();
    m_pFrame->detach();
    delete m_pFrame;
    m_pFrame = 0L;
  }
  m_vView->disconnectObject( m_vParent );
  m_vView->decRef(); //die view, die ... (cruel world, isn't it?) ;)
  VeryBadHackToFixCORBARefCntBug( m_vView );
  m_vView = 0L; //now it _IS_ dead
}

void KonqChildView::repaint()
{
  kdebug(0,1202,"KonqChildView::repaint()");
  if (m_pFrame != 0L) m_pFrame->repaint();
  m_pHeader->repaint();
  kdebug(0,1202,"KonqChildView::repaint() : done");
}

void KonqChildView::show()
{
  kdebug(0,1202,"KonqChildView::show()");
  m_pWidget->show();
  m_vView->show(true);
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

void KonqChildView::emitMenuEvents( OpenPartsUI::Menu_ptr viewMenu, OpenPartsUI::Menu_ptr editMenu, bool create )
{
  Konqueror::View::EventFillMenu ev;
  ev.menu = OpenPartsUI::Menu::_duplicate( viewMenu );
  ev.create = create;
  EMIT_EVENT( m_vView, Konqueror::View::eventFillMenuView, ev );
  ev.menu = OpenPartsUI::Menu::_duplicate( editMenu );
  EMIT_EVENT( m_vView, Konqueror::View::eventFillMenuEdit, ev );
}

void KonqChildView::switchView( Konqueror::View_ptr _vView, const QStringList &serviceTypes )
{
  kdebug(0,1202,"switchView : part->inactive");
  m_vMainWindow->setActivePart( m_vParent->id() );
    
  detach();
  Konqueror::View_var vView = Konqueror::View::_duplicate( _vView );
  kdebug(0,1202,"switchView : attaching new one");
  attach( vView );

  m_lstServiceTypes = serviceTypes;
  
  show(); // switchView is never called on startup. We can always show() the view.
}

bool KonqChildView::changeViewMode( const QString &serviceType, const QString &_url )
{
  QString url = _url;
  if ( url.isEmpty() )
    url = KonqChildView::url();
    
  //no need to change anything if we are able to display this servicetype
  if ( m_lstServiceTypes.find( serviceType ) != m_lstServiceTypes.end() )
  {
    openURL( url );
    return true;
  }

  Konqueror::View_var vView;
  QStringList serviceTypes;
  if ( !createView( serviceType, vView, serviceTypes ) )
   return false;
  
  OpenParts::Id oldId = m_vView->id();
  switchView( vView, serviceTypes );
  
  emit sigIdChanged( this, oldId, vView->id() );
  
  openURL( url );
  
  m_vMainWindow->setActivePart( vView->id() ); 
  return true;
}

void KonqChildView::changeView( Konqueror::View_ptr _vView, const QStringList &serviceTypes, const QString &_url )
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
    if ( !m_bHistoryLock && ( url != m_tmpInternalHistoryEntry.strURL ) ) // no lock
    {
      if ( m_bBack )
      {
        m_bBack = false;
        kdebug(0,1202,"pushing into forward history : %s", m_tmpInternalHistoryEntry.strURL.ascii() );
        m_lstForward.push_front( m_tmpInternalHistoryEntry );
      }
      else if ( m_bForward )
      {
        m_bForward = false;
        kdebug(0,1202,"pushing into backward history : %s", m_tmpInternalHistoryEntry.strURL.ascii() );
        m_lstBack.push_back( m_tmpInternalHistoryEntry );
      }
      else
      {
        m_lstForward.clear();
        kdebug(0,1202,"pushing into backward history : %s", m_tmpInternalHistoryEntry.strURL.ascii() );
        m_lstBack.push_back( m_tmpInternalHistoryEntry );
      }	
    }
    else
      m_bHistoryLock = false;
  
    h.bHasHistoryEntry = false;
    h.strURL = url;
    h.strServiceType = m_lstServiceTypes.first();
    m_tmpInternalHistoryEntry = h;
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
  kdebug(0,1202,"restoring %s with stype %s", h.entry.url.in(), h.strServiceType.ascii());

  if ( h.bHasHistoryEntry )  
  {
    Konqueror::View_var vView;
    QStringList serviceTypes;
    createView( h.strServiceType, vView, serviceTypes );
    
    OpenParts::Id oldId = m_vView->id();
    switchView( vView, serviceTypes );

    emit sigIdChanged( this, oldId, vView->id() );
  
    vView->restoreState( h.entry );
    
    m_vMainWindow->setActivePart( vView->id() );    
  }    
  else
  {
    kdebug(0,1202,"restoring %s",h.strURL.data());
    changeViewMode( h.strServiceType, h.strURL );
  }
}

void KonqChildView::goForward()
{
  assert( m_lstForward.size() != 0 );

  InternalHistoryEntry h = m_lstForward.front();
  m_lstForward.pop_front();
  m_bForward = true;
  kdebug(0,1202,"restoring %s with stype %s", h.entry.url.in(), h.strServiceType.ascii());

  if ( h.bHasHistoryEntry )  
  {
    Konqueror::View_var vView;
    QStringList serviceTypes;
    createView( h.strServiceType, vView, serviceTypes );
    
    OpenParts::Id oldId = m_vView->id();
    switchView( vView, serviceTypes );

    emit sigIdChanged( this, oldId, vView->id() );
  
    vView->restoreState( h.entry );
    
    m_vMainWindow->setActivePart( vView->id() );    
  }    
  else
    changeViewMode( h.strServiceType, h.strURL );
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

bool KonqChildView::supportsServiceType( const QString &serviceType )
{
  bool result = m_lstServiceTypes.contains( serviceType );
  kdebug(0, 1202, "KonqChildView::supportsServiceType(%s) : returning %d", serviceType.data(), result);
  return result;
}

void KonqChildView::slotHeaderClicked()
{
  m_vMainWindow->setActivePart( m_vView->id() );
}

bool KonqChildView::createView( const QString &serviceType, Konqueror::View_var &view, QStringList &serviceTypes )
{
  serviceTypes.clear();

  kdebug(0,1202,"trying to create view for %s", serviceType.ascii());
  
  //check for builtin views first
  if ( serviceType == "inode/directory" )
  {
    //default for directories is the iconview
    view = Konqueror::View::_duplicate( new KonqKfmIconView );
    serviceTypes.append( serviceType );
    return true;
  }
  else if ( serviceType == "text/html" )
  {
    view = Konqueror::View::_duplicate( new KonqHTMLView );
    serviceTypes.append( serviceType );
    return true;
  }
  else if ( serviceType.left( 5 ) == "text/" &&
            ( serviceType.mid( 5, 2 ) == "x-" ||
	      serviceType.mid( 5 ) == "english" ||
	      serviceType.mid( 5 ) == "plain" ) )
  {
    view = Konqueror::View::_duplicate( new KonqTxtView );
    serviceTypes.append( serviceType );
    return true;
  }
  
  //now let's query the Trader for view plugins
  KTrader *trader = KdedInstance::self()->ktrader();
  KActivator *activator = KdedInstance::self()->kactivator();
  
  KTrader::OfferList offers = trader->query( serviceType, "'Konqueror/View' in ServiceTypes" );
  
  if ( offers.count() == 0 ) //no results?
    return false;
    
  //activate the view plugin
  KTrader::ServicePtr service = offers.first();
  
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
  
  Konqueror::ViewFactory_var factory = Konqueror::ViewFactory::_narrow( obj );
  
  if ( CORBA::is_nil( factory ) )
    return false;

  view = factory->create();

  if ( CORBA::is_nil( view ) )
    return false;

  serviceTypes = service->serviceTypes();

  return true;
}

#include "konq_childview.moc"
