#ifndef __KICON_DRAG_H__
#define __KICON_DRAG_H__

#include <qdragobject.h>
#include <qvaluelist.h>
#include <qstring.h>
#include <qpoint.h>

#undef Icon

class KIconDrag : public QDragObject
{
  Q_OBJECT
public:
  struct Icon
  {
    Icon() { };
    Icon( const QString& _url, const QPoint& _pos ) { url = _url; pos = _pos; }

    QString url;
    QPoint pos;
  };

  typedef QValueList<Icon> IconList;

  KIconDrag( const IconList &_icons, QWidget * dragSource, const char* _name = 0 );
  KIconDrag( QWidget * dragSource, const char* _name = 0 );
  ~KIconDrag();

  void setIcons( const IconList &_list );
  void append( const Icon& _icon ) { m_lstIcons.append( _icon ); }

  const char* format(int i) const;
  QByteArray encodedData(const char* mime) const;
  
  static bool canDecode( QMimeSource* e );
  static bool decode( QMimeSource* e, IconList& _list );
  static bool decode( QMimeSource* e, QStringList& _list );

protected:
  IconList m_lstIcons;
};

#endif
