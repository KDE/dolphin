/* This file is part of the KDE project
   Copyright (C) 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#ifndef __KICON_DRAG_H__
#define __KICON_DRAG_H__

#include <qdragobject.h>
#include <qvaluelist.h>
#include <qstring.h>
#include <qpoint.h>

#undef Icon

/**
 * This class defines the drag object formed by one or several icons
 * so that icons can be moved around or dropped into another application
 */
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
  
  /**
   *  Attempts to decode the dropped information in e
   *  into _list, returning TRUE if successful.
   */
  static bool decode( QMimeSource* e, IconList& _list );
  static bool decode( QMimeSource* e, QStringList& _list );

protected:
  IconList m_lstIcons;
};

#endif
