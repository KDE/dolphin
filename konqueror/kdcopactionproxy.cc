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

#include "kdcopactionproxy.h"

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

#include <dcopclient.h>
#include <kapp.h>
#include <kaction.h>
#include <kdebug.h>

#include <ctype.h>

class KDCOPActionProxy::KDCOPActionProxyPrivate
{
public:
  KDCOPActionProxyPrivate()
  {
  }
  ~KDCOPActionProxyPrivate()
  {
  }

  KActionCollection *m_actionCollection;
  DCOPObject *m_parent;
  QCString m_prefix;
  int m_pos;
};

KDCOPActionProxy::KDCOPActionProxy( KActionCollection *actionCollection, DCOPObject *parent )
{
  d = new KDCOPActionProxyPrivate;
  d->m_actionCollection = actionCollection;
  d->m_parent = parent;
  d->m_prefix = parent->objId() + "/action/";
  d->m_pos = d->m_prefix.length();
}

KDCOPActionProxy::~KDCOPActionProxy()
{
  delete d;
}

QCString KDCOPActionProxy::action( const QString &name ) const
{
  return d->m_prefix + name.latin1();
}

QMap<QString,DCOPRef> KDCOPActionProxy::actionMap( const QCString &appId ) const
{
  QMap<QString,DCOPRef> res;

  QCString id = appId;
  if ( id.isEmpty() )
    id = kapp->dcopClient()->appId();

  QValueList<KAction *> actions = d->m_actionCollection->actions();
  QValueList<KAction *>::ConstIterator it = actions.begin();
  QValueList<KAction *>::ConstIterator end = actions.end();
  for (; it != end; ++it )
  {
    QString name = QString::fromLatin1( (*it)->name() );
    res.insert( name, DCOPRef( id, action( name ) ) );
  }

  return res;
}

bool KDCOPActionProxy::process( const QCString &obj, const QCString &fun, const QByteArray &data,
			        QCString &replyType, QByteArray &replyData )
{
  if ( obj.left( d->m_pos ) != d->m_prefix )
    return false;

  KAction *action = d->m_actionCollection->action( obj.mid( d->m_pos ) );
  if ( !action )
    return false;

  return processAction( obj, fun, data, replyType, replyData, action );
}

bool KDCOPActionProxy::processAction( const QCString &, const QCString &fun, const QByteArray &data,
			              QCString &replyType, QByteArray &replyData, KAction *action )
{
  if ( fun == "activate()" )
  {
    replyType = "void";
    action->activate();
    return true;
  }

  if ( fun == "isPlugged()" )
  {
    replyType = "bool";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << (Q_INT8)action->isPlugged();
    return true;
  }

  if ( fun == "functions()" )
  {
    QCString s = "functions();activate();isPlugged();";
    
    QStrList properties = action->metaObject()->propertyNames( true );
    QStrListIterator it( properties );
    for (; it.current(); ++it )
    {
      QCString name = it.current();
      name.append( "();" );
      QCString setName = name.copy();
      setName[ 0 ] = toupper( setName[ 0 ] );
      setName.prepend( "set" );
      
      s += name;
      s += setName;
    }
  
    replyType = "QCString";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << s;
    return true;
  }
  
  if ( fun.length() < 3 )
    return false;

  bool set = false;

  QCString name = fun;
  QCString arg;

  if ( name.left( 3 ) == "set" )
  {
    name.detach();
    set = true;
    name = name.mid( 3 );
    int p1 = name.find( '(' );

    uint len = name.length();

    if ( name[ len - 1 ] != ')' )
      return false;

    arg = name.mid( p1+1, len - p1 - 2 );
    name.truncate( p1 );
    name[ 0 ] = tolower( name[ 0 ] );
  }
  else
    name.truncate( name.length() - 2 );

  if ( !action->metaObject()->propertyNames( true ).contains( name ) )
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
    return action->setProperty( name, prop );
  }
  else
  {
    QVariant prop = action->property( name );

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

