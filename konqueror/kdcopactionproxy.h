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
#ifndef __kdcopactionproxy_h__
#define __kdcopactionproxy_h__

#include <dcopobject.h>
#include <dcopref.h>

class KActionCollection;
class KAction;

class KActionInterface
{
};

class KDCOPActionProxy : public DCOPObjectProxy
{
public:
  KDCOPActionProxy( KActionCollection *actionCollection, DCOPObject *parent );
  ~KDCOPActionProxy();

  virtual QValueList<KAction *> actions() const;
  virtual KAction *action( const char *name ) const;

  virtual QCString actionObjectId( const QString &name ) const;

  virtual QMap<QString,DCOPRef> actionMap( const QCString &appId = QCString() ) const;

  virtual bool process( const QCString &obj, const QCString &fun, const QByteArray &data,
			QCString &replyType, QByteArray &replyData );

  virtual bool processAction( const QCString &obj, const QCString &fun, const QByteArray &data,
			      QCString &replyType, QByteArray &replyData, KAction *action );
private:
  class KDCOPActionProxyPrivate;
  KDCOPActionProxyPrivate *d;
};

#endif
