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
#ifndef __kdcoppropertyproxy_h__
#define __kdcoppropertyproxy_h__

#include <qobject.h>
#include <qcstring.h>

class KDCOPPropertyProxy
{
public:
  KDCOPPropertyProxy( QObject *object );
  ~KDCOPPropertyProxy();

  bool isPropertyRequest( const QCString &fun );

  bool processPropertyRequest( const QCString &fun, const QByteArray &data, QCString &replyType,
			       QByteArray &replyData );

  QCString functions();

  static QCString functions( QObject *object );

  static bool isPropertyRequest( const QCString &fun, QObject *object );

  static bool processPropertyRequest( const QCString &fun, const QByteArray &data, QCString &replyType,
				      QByteArray &replyData, QObject *object );

private:
  static bool decodePropertyRequestInternal( const QCString &fun, QObject *object, bool &set,
					     QCString &propName, QCString &arg );

  class KDCOPPropertyProxyPrivate;
  KDCOPPropertyProxyPrivate *d;
};

#endif
