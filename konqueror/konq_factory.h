/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __konq_factory_h__
#define __konq_factory_h__ $Id$

#include <qstring.h>
#include <qstringlist.h>

#include <klibloader.h>
#include <kinstance.h>
#include <ktrader.h>

class BrowserView;
class KonqBookmarkManager;
class KonqFileManager;
class KonqMainView;

class KonqViewFactory
{
public:
  KonqViewFactory() : m_factory( 0L ) {}
  KonqViewFactory( KLibFactory *factory, const QStringList &args ) : m_factory( factory ), m_args( args ) {}
  KonqViewFactory( const KonqViewFactory &factory ) : m_factory( factory.m_factory ), m_args( factory.m_args ) {}
  KonqViewFactory( KonqViewFactory &factory ) : m_factory( factory.m_factory ), m_args( factory.m_args ) {}

  BrowserView *create( QWidget *parent, const char *name );

  bool isNull() { return m_factory ? false : true; }

private:
  KLibFactory *m_factory;
  QStringList m_args;
};

class KonqFactory : public KLibFactory
{
  Q_OBJECT
public:
  KonqFactory();
  virtual ~KonqFactory();

  static KonqViewFactory createView( const QString &serviceType,
				     const QString &serviceName = QString::null,
				     KService::Ptr *serviceImpl = 0,
				     KTrader::OfferList *serviceOffers = 0 );
				

  virtual QObject* create( QObject* parent = 0, const char* name = 0, const char* classname = "QObject", const QStringList &args = QStringList() );

  static void instanceRef();
  static void instanceUnref();

  static KInstance *instance();	

private:
  static unsigned long m_instanceRefCnt;
  static KInstance *s_instance;
  KonqBookmarkManager *m_bookmarkManager;
  KonqFileManager *m_fileManager;
};

#endif
