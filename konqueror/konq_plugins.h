/*  This file is part of the KDE project
    Copyright (C) 1997 Simon Hausmann <hausmann@kde.org>
 
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

#ifndef __konq_plugins_h__
#define __konq_plugins_h__

#include <qstring.h>
#include <qmap.h>

#include <kom.h>

class KonqPlugins
{
public:
  static CORBA::Object_ptr lookupViewServer( const QString serviceType );
  
  static void installKOMPlugins( KOM::Component_ptr comp );
  
  /**
   * Associate a view-name with a service type. When a plugin view is created, we need
   * to store its name and service here, in case we want to create another one from the
   * view name (e.g. "split")
   */
  static void associate( const QString &viewName, const QString &serviceType );

  /**
   * @return the service type that the view named 'viewName' handles
   */
  static QString getServiceType( const QString &viewName );

private:
  /* Maps view names to service types (only for plugin views!!) */
  static QMap<QString,QString> s_mapServiceTypes;
};

#endif
