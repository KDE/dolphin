
#include "konq_baseview.h"
#include <kdebug.h>

KonqBaseView::KonqBaseView()
{
  ADD_INTERFACE( "IDL:Konqueror/View:1.0" );

  SIGNAL_IMPL( "openURL" );
  SIGNAL_IMPL( "started" );
  SIGNAL_IMPL( "completed" );
  SIGNAL_IMPL( "canceled" );
  SIGNAL_IMPL( "setStatusBarText" );
  SIGNAL_IMPL( "setLocationBarURL" );
  SIGNAL_IMPL( "createNewWindow" );
  
  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );
  
  m_strURL = "";
}

KonqBaseView::~KonqBaseView()
{
  kdebug(0,1202,"KonqBaseView::~KonqBaseView() -> %s", debug_ViewName.in());
  cleanUp();
}

void KonqBaseView::init()
{
  debug_ViewName = viewName();
}

void KonqBaseView::cleanUp()
{
  if ( m_bIsClean )
    return;

  kdebug(0,1202,"void KonqBaseView::cleanUp() -> %s", debug_ViewName.in());

  OPPartIf::cleanUp();
}

bool KonqBaseView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Konqueror::View::eventFillMenuEdit, Konqueror::View::EventFillMenu, mappingFillMenuEdit );
  MAPPING( Konqueror::View::eventFillMenuView, Konqueror::View::EventFillMenu, mappingFillMenuView );
  MAPPING( Konqueror::View::eventCreateViewToolBar, Konqueror::View::EventCreateViewToolBar, mappingCreateViewToolBar );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}

bool KonqBaseView::mappingFillMenuView( Konqueror::View::EventFillMenu )
{
  return false;
}

bool KonqBaseView::mappingFillMenuEdit( Konqueror::View::EventFillMenu )
{
  return false;
}

bool KonqBaseView::mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar )
{
  return false;
}

bool KonqBaseView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  SIGNAL_CALL2( "setLocationBarURL", id(), (char*)eventURL.url );
  return false;
}

char *KonqBaseView::url()
{
  return CORBA::string_dup( m_strURL.data() );
}

Konqueror::View::HistoryEntry *KonqBaseView::saveState()
{
  Konqueror::View::HistoryEntry *entry = new Konqueror::View::HistoryEntry;
  
  entry->url = url();
  
  return entry;
}

void KonqBaseView::restoreState( const Konqueror::View::HistoryEntry &entry )
{
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( entry.url );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( this, Konqueror::eventOpenURL, eventURL );
}

void KonqBaseView::openURLRequest( const char *_url )
{
  // Ask the main view to open this URL, since it might not be suitable
  // for the current type of view. It might even be a file (KRun will be used)
  Konqueror::URLRequest urlRequest;
  urlRequest.url = CORBA::string_dup( _url );
  urlRequest.reload = (CORBA::Boolean)false;
  urlRequest.xOffset = 0;
  urlRequest.yOffset = 0;
  SIGNAL_CALL1( "openURL", urlRequest );
}
