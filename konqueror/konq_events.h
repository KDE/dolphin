#ifndef __konq_events_h__
#define __konq_events_h__ $Id$

#include <kevent.h>

class KonqURLEnterEvent : public KParts::Event
{
public:
  KonqURLEnterEvent( const QString &url ) : KParts::Event( s_urlEnterEventName ), m_url( url ) {};

  QString url() const { return m_url; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_urlEnterEventName ); }

private:
  static const char *s_urlEnterEventName;

  QString m_url;
};

#endif
