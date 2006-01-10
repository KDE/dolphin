#ifndef __konq_events_h__
#define __konq_events_h__

#include <kparts/event.h>
#include <QList>
#include <QEvent>
#include <libkonq_export.h>

namespace KParts
{
  class ReadOnlyPart;
}

class KConfig;
class KFileItem;
class KFileItemList;

class LIBKONQ_EXPORT KonqFileSelectionEvent : public KParts::Event
{
public:
  KonqFileSelectionEvent( const KFileItemList &selection, KParts::ReadOnlyPart *part ) : KParts::Event( s_fileItemSelectionEventName ), m_selection( selection ), m_part( part ) {}

  KFileItemList selection() const { return m_selection; }
  KParts::ReadOnlyPart *part() const { return m_part; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_fileItemSelectionEventName ); }

private:
  static const char *s_fileItemSelectionEventName;

  KFileItemList m_selection;
  KParts::ReadOnlyPart *m_part;
};

class LIBKONQ_EXPORT KonqFileMouseOverEvent : public KParts::Event
{
public:
  KonqFileMouseOverEvent( const KFileItem *item, KParts::ReadOnlyPart *part ) : KParts::Event( s_fileItemMouseOverEventName ), m_item( item ), m_part( part ) {}

  const KFileItem* item() const { return m_item; }
  KParts::ReadOnlyPart *part() const { return m_part; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_fileItemMouseOverEventName ); }

private:
  static const char *s_fileItemMouseOverEventName;

  const KFileItem* m_item;
  KParts::ReadOnlyPart *m_part;
};

class LIBKONQ_EXPORT KonqConfigEvent : public KParts::Event
{
public:
  KonqConfigEvent( KConfig *config, const QString &prefix, bool save ) : KParts::Event( s_configEventName ), m_config( config ), m_prefix( prefix ), m_save( save ) {}

  KConfig * config() const { return m_config; }
  QString prefix() const { return m_prefix; }
  bool save() const { return m_save; }

  static bool test( const QEvent *event ) { return KParts::Event::test( event, s_configEventName ); }

private:
  static const char *s_configEventName;

  KConfig *m_config;
  QString m_prefix;
  bool m_save;
};

#endif
