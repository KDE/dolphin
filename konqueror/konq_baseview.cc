
#include "konq_baseview.h"

KonqBaseView::KonqBaseView()
{
  SIGNAL_IMPL( "openURL" );
  SIGNAL_IMPL( "started" );
  SIGNAL_IMPL( "completed" );
  SIGNAL_IMPL( "canceled" );
  SIGNAL_IMPL( "setStatusBarText" );
  SIGNAL_IMPL( "setLocationBarURL" );
  SIGNAL_IMPL( "createNewWindow" );
  SIGNAL_IMPL( "popupMenu" );
  SIGNAL_IMPL( "addHistory" );
  
  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );
  
  m_strURL = "";
  m_strTitle = "";
}

KonqBaseView::~KonqBaseView()
{
  cleanUp();
}

void KonqBaseView::init()
{
}

void KonqBaseView::cleanUp()
{
  if ( m_bIsClean )
    return;

  OPPartIf::cleanUp();
}

bool KonqBaseView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Konqueror::View::eventCreateViewMenuBar, Konqueror::View::EventCreateViewMenuBar, mappingCreateViewMenuBar );
  MAPPING( Konqueror::View::eventCreateViewToolBar, Konqueror::View::EventCreateViewToolBar, mappingCreateViewToolBar );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}

bool KonqBaseView::mappingCreateViewMenuBar( Konqueror::View::EventCreateViewMenuBar viewMenuStruct )
{
  return false;
}

bool KonqBaseView::mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar viewToolBar )
{
  return false;
}

bool KonqBaseView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  SIGNAL_CALL1( "setLocationBarURL", (char*)eventURL.url );
  return false;
}

char *KonqBaseView::url()
{
  return CORBA::string_dup( m_strURL.data() );
}

char *KonqBaseView::title()
{
  return CORBA::string_dup( m_strTitle.data() );
}
