#ifndef __konq_events_h__
#define __konq_events_h__ $Id$

#include <kparts/event.h>
#include <qlist.h>

namespace KParts
{
  class ReadOnlyPart;
};

class KonqFileItem;
typedef QList<KonqFileItem> KonqFileItemList;

class KonqURLEnterEvent : public KParts::Event
{
public:
  KonqURLEnterEvent( const QString &url ) : KParts::Event( s_urlEnterEventName ), m_url( url ) {}

  QString url() const { return m_url; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_urlEnterEventName ); }

private:
  static const char *s_urlEnterEventName;

  QString m_url;
};

class KonqFileSelectionEvent : public KParts::Event
{
public:
  KonqFileSelectionEvent( const KonqFileItemList &selection, KParts::ReadOnlyPart *part ) : KParts::Event( s_fileItemSelectionEventName ), m_selection( selection ), m_part( part ) {}

  KonqFileItemList selection() const { return m_selection; }
  KParts::ReadOnlyPart *part() const { return m_part; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_fileItemSelectionEventName ); }

private:
  static const char *s_fileItemSelectionEventName;

  KonqFileItemList m_selection;
  KParts::ReadOnlyPart *m_part;
};

#endif
