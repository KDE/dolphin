/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#include "kdcoppropertyproxy.h"

#include <qstrlist.h>
#include <qmetaobject.h>
#include <qvariant.h>
#include <qcursor.h>
#include <qbitmap.h>
#include <qregion.h>
#include <qpointarray.h>
#include <qiconset.h>
#include <qfont.h>
#include <qimage.h>
#include <qbrush.h>
#include <qpalette.h>

#include <ctype.h>
#include <assert.h>

class KDCOPPropertyProxy::KDCOPPropertyProxyPrivate
{
public:
  KDCOPPropertyProxyPrivate()
  {
  }
  ~KDCOPPropertyProxyPrivate()
  {
  }

  QObject *m_object;
};

KDCOPPropertyProxy::KDCOPPropertyProxy( QObject *object )
{
  d = new KDCOPPropertyProxyPrivate;
  d->m_object = object;
}

KDCOPPropertyProxy::~KDCOPPropertyProxy()
{
  delete d;
}

bool KDCOPPropertyProxy::isPropertyRequest( const QCString &fun )
{
  return isPropertyRequest( fun, d->m_object );
}

bool KDCOPPropertyProxy::processPropertyRequest( const QCString &fun, const QByteArray &data,
						 QCString &replyType, QByteArray &replyData )
{
  return processPropertyRequest( fun, data, replyType, replyData, d->m_object );
}

QCString KDCOPPropertyProxy::functions()
{
  return functions( d->m_object );
}

bool KDCOPPropertyProxy::isPropertyRequest( const QCString &fun, QObject *object )
{
  if ( fun == "property(QCString)" ||
       fun == "setProperty(QCString,QVariant)" ||
       fun == "propertyNames(bool)" )
    return true;

  bool set;
  QCString propName, arg;
  return decodePropertyRequestInternal( fun, object, set, propName, arg );
}

QCString KDCOPPropertyProxy::functions( QObject *object )
{
  QCString res = "property(QCString);setProperty(QCString,QVariant);propertyNames(bool);";

  QMetaObject *metaObj = object->metaObject();
  QStrList properties = metaObj->propertyNames( true );
  QStrListIterator it( properties );
  for (; it.current(); ++it )
  {
    QCString name = it.current();
    name.append( "();" );
    res += name;

    const QMetaProperty *metaProp = metaObj->property( it.current(), true );
    assert( metaProp );
    if ( metaProp->writeable() )
    {
      QCString setName = name.copy();
      setName[ 0 ] = toupper( setName[ 0 ] );
      setName.prepend( "set" );
      res += setName;
    }
  }

  return res;
}

bool KDCOPPropertyProxy::processPropertyRequest( const QCString &fun, const QByteArray &data,
						 QCString &replyType, QByteArray &replyData,
						 QObject *object )
{
  if ( fun == "property(QCString)" )
  {
    QCString propName;
    QDataStream stream( data, IO_ReadOnly );
    stream >> propName;

    replyType = "QVariant";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << object->property( propName );
    return true;
  }

  if ( fun == "setProperty(QCString,QVariant)" )
  {
    QCString propName;
    QVariant propValue;
    QDataStream stream( data, IO_ReadOnly );
    stream >> propName >> propValue;

    replyType = "bool";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << (Q_INT8)object->setProperty( propName, propValue );
    return true;
  }

  if ( fun == "propertyNames(bool)" )
  {
    Q_INT8 b;
    QDataStream stream( data, IO_ReadOnly );
    stream >> b;

    QValueList<QCString> res;
    QStrList props = object->metaObject()->propertyNames( static_cast<bool>( b ) );
    QStrListIterator it( props );
    for (; it.current(); ++it )
      res.append( it.current() );

    replyType = "QValueList<QCString>";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << res;
    return true;
  }

  bool set;
  QCString propName, arg;

  bool res = decodePropertyRequestInternal( fun, object, set, propName, arg );
  if ( !res )
    return false;

  if ( set )
  {
    QVariant prop;
    QDataStream stream( data, IO_ReadOnly );

    QVariant::Type type = QVariant::nameToType( arg );
    if ( type == QVariant::Invalid )
      return false;

#define DEMARSHAL( type, val ) \
  case QVariant::##type: \
    { \
      val v; \
      stream >> v; \
      prop = QVariant( v ); \
    } \
    break;

    typedef QValueList<QVariant> ListType;
    typedef QMap<QString,QVariant> MapType;

    switch ( type )
    {
      DEMARSHAL( Cursor, QCursor )
      DEMARSHAL( Bitmap, QBitmap )
      DEMARSHAL( PointArray, QPointArray )
      DEMARSHAL( Region, QRegion )
      DEMARSHAL( List, ListType )
      DEMARSHAL( Map, MapType )
      DEMARSHAL( String, QString )
      DEMARSHAL( CString, QCString )
      DEMARSHAL( StringList, QStringList )
      DEMARSHAL( Font, QFont )
      DEMARSHAL( Pixmap, QPixmap )
      DEMARSHAL( Image, QImage )
      DEMARSHAL( Brush, QBrush )
      DEMARSHAL( Point, QPoint )
      DEMARSHAL( Rect, QRect )
      DEMARSHAL( Size, QSize )
      DEMARSHAL( Color, QColor )
      DEMARSHAL( Palette, QPalette )
      DEMARSHAL( ColorGroup, QColorGroup )
      case QVariant::IconSet:
      {
        QPixmap val;
	stream >> val;
        prop = QVariant( QIconSet( val ) );
      }
      break;
      DEMARSHAL( Int, int )
      DEMARSHAL( UInt, uint )
      case QVariant::Bool:
      {
        Q_INT8 v;
	stream >> v;
        prop = QVariant( static_cast<bool>( v ), 1 );
      }
        break;
      DEMARSHAL( Double, double )
      default:
        return false;
        break;
    }

    replyType = "void";
    return object->setProperty( propName, prop );
  }
  else
  {
    QVariant prop = object->property( propName );

    if ( prop.type() == QVariant::Invalid )
      return false;

    replyType = prop.typeName();
    QDataStream reply( replyData, IO_WriteOnly );

#define MARSHAL( type ) \
  case QVariant::##type: \
    reply << prop.to##type(); \
    break;

    switch ( prop.type() )
    {
      MARSHAL( Cursor )
      MARSHAL( Bitmap )
      MARSHAL( PointArray )
      MARSHAL( Region )
      MARSHAL( List )
      MARSHAL( Map )
      MARSHAL( String )
      MARSHAL( CString )
      MARSHAL( StringList )
      MARSHAL( Font )
      MARSHAL( Pixmap )
      MARSHAL( Image )
      MARSHAL( Brush )
      MARSHAL( Point )
      MARSHAL( Rect )
      MARSHAL( Size )
      MARSHAL( Color )
      MARSHAL( Palette )
      MARSHAL( ColorGroup )
      case QVariant::IconSet:
        reply << prop.toIconSet().pixmap();
        break;
      MARSHAL( Int )
      MARSHAL( UInt )
      case QVariant::Bool:
        reply << (Q_INT8)prop.toBool();
        break;
      MARSHAL( Double )
      default:
        return false;
        break;
    }
	
#undef MARSHAL
#undef DEMARSHAL

    return true;
  }

  return false;
}

bool KDCOPPropertyProxy::decodePropertyRequestInternal( const QCString &fun, QObject *object, bool &set,
							QCString &propName, QCString &arg )
{
  if ( fun.length() < 3 )
    return false;

  set = false;

  propName = fun;

  if ( propName.left( 3 ) == "set" )
  {
    propName.detach();
    set = true;
    propName = propName.mid( 3 );
    int p1 = propName.find( '(' );

    uint len = propName.length();

    if ( propName[ len - 1 ] != ')' )
      return false;

    arg = propName.mid( p1+1, len - p1 - 2 );
    propName.truncate( p1 );
    propName[ 0 ] = tolower( propName[ 0 ] );
  }
  else
    propName.truncate( propName.length() - 2 );

  if ( !object->metaObject()->propertyNames( true ).contains( propName ) )
    return false;

  return true;
}
