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

#include <dcopclient.h>
#include <kapp.h>
#include <kaction.h>
#include <kdebug.h>
#include <kdcoppropertyproxy.h>

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

QValueList<KAction *>KDCOPActionProxy::actions() const
{
  return d->m_actionCollection->actions();
}

KAction *KDCOPActionProxy::action( const char *name ) const
{
  return d->m_actionCollection->action( name );
}

QCString KDCOPActionProxy::actionObjectId( const QString &name ) const
{
  return d->m_prefix + name.latin1();
}

QMap<QString,DCOPRef> KDCOPActionProxy::actionMap( const QCString &appId ) const
{
  QMap<QString,DCOPRef> res;

  QCString id = appId;
  if ( id.isEmpty() )
    id = kapp->dcopClient()->appId();

  QValueList<KAction *> lst = actions();
  QValueList<KAction *>::ConstIterator it = lst.begin();
  QValueList<KAction *>::ConstIterator end = lst.end();
  for (; it != end; ++it )
  {
    QString name = QString::fromLatin1( (*it)->name() );
    res.insert( name, DCOPRef( id, actionObjectId( name ) ) );
  }

  return res;
}

bool KDCOPActionProxy::process( const QCString &obj, const QCString &fun, const QByteArray &data,
			        QCString &replyType, QByteArray &replyData )
{
  if ( obj.left( d->m_pos ) != d->m_prefix )
    return false;

  KAction *act = action( obj.mid( d->m_pos ) );
  if ( !act )
    return false;

  return processAction( obj, fun, data, replyType, replyData, act );
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

    s += KDCOPPropertyProxy::functions( action );

    replyType = "QCString";
    QDataStream reply( replyData, IO_WriteOnly );
    reply << s;
    return true;
  }

  return KDCOPPropertyProxy::processPropertyRequest( fun, data, replyType, replyData, action );
}

