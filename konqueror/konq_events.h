#ifndef __konq_events_h__
#define __konq_events_h__ $Id$

#include <qevent.h>

#define KONQ_EVENT_MAGIC 2437

class KonqEvent : public QCustomEvent
{
public:
  KonqEvent() : QCustomEvent( (QEvent::Type)(QEvent::User + KONQ_EVENT_MAGIC), (void *)0 ) {};
  virtual ~KonqEvent() {};

  virtual const char *eventName() const = 0;
};

class KonqURLEnterEvent : public KonqEvent
{
public:
  KonqURLEnterEvent( const QString &url ) : m_url( url ) {};
  virtual ~KonqURLEnterEvent() {};

  QString url() const { return m_url; }

  virtual const char *eventName() const { return s_urlEnterEventName; }

  static bool test( QEvent *event );

private:
  static const char *s_urlEnterEventName;

  QString m_url;
};

#endif
