
#include "konq_baseview.h"

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
  SIGNAL_IMPL( "popupMenu" );
  
  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );
  
  m_strURL = "";
}

KonqBaseView::~KonqBaseView()
{
  cerr << "KonqBaseView::~KonqBaseView() -> " << debug_ViewName.in() << endl;
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

  cerr << "void KonqBaseView::cleanUp() -> " << debug_ViewName.in() << endl;

  OPPartIf::cleanUp();
}

bool KonqBaseView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Konqueror::View::eventCreateViewMenu, Konqueror::View::EventCreateViewMenu, mappingCreateViewMenu );
  MAPPING( Konqueror::View::eventCreateViewToolBar, Konqueror::View::EventCreateViewToolBar, mappingCreateViewToolBar );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}

bool KonqBaseView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
  return false;
}

bool KonqBaseView::mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar viewToolBar )
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
