
#include "konq_events.h"

const char *KonqURLEnterEvent::s_urlEnterEventName = "Konqueror/URLEntered";

bool KonqURLEnterEvent::test( QEvent *e )
{
  if ( e->type() == ( QEvent::User + KONQ_EVENT_MAGIC ) &&
       strcmp( ((KonqEvent *)e)->eventName(), s_urlEnterEventName ) == 0 )
    return true;

  return false;
}
