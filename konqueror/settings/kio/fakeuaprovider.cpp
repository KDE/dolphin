/*
 * Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <time.h>
#include <sys/utsname.h>


#include <kdebug.h>
#include <klocale.h>
#include <kservicetypetrader.h>
#include <kstandarddirs.h>

#include "fakeuaprovider.h"

#define UA_PTOS(x) (*it)->property(x).toString()
#define QFL(x) QLatin1String(x)

FakeUASProvider::FakeUASProvider()
{
   m_bIsDirty = true;
}

FakeUASProvider::StatusCode FakeUASProvider::createNewUAProvider( const QString& uaStr )
{
  QStringList split;
  int pos = (uaStr).indexOf("::");

  if ( pos == -1 )
  {
    pos = (uaStr).indexOf(':');
    if ( pos != -1 )
    {
      split.append(uaStr.left(pos));
      split.append(uaStr.mid(pos+1));
    }
  }
  else
  {
    split = uaStr.split( "::");
  }

  if ( m_lstIdentity.contains(split[1]) )
    return DUPLICATE_ENTRY;
  else
  {
    int count = split.count();
    m_lstIdentity.append( split[1] );
    if ( count > 2 )
      m_lstAlias.append(split[2]);
    else
      m_lstAlias.append( split[1]);
  }

  return SUCCEEDED;
}

void FakeUASProvider::loadFromDesktopFiles()
{
  m_providers.clear();
  m_providers = KServiceTypeTrader::self()->query("UserAgentStrings");
}

void FakeUASProvider::parseDescription()
{
  QString tmp;

  KService::List::ConstIterator it = m_providers.begin();
  KService::List::ConstIterator lastItem = m_providers.end();

  for ( ; it != lastItem; ++it )
  {
    tmp = UA_PTOS("X-KDE-UA-FULL");

    if ( (*it)->property("X-KDE-UA-DYNAMIC-ENTRY").toBool() )
    {
      struct utsname utsn;
      uname( &utsn );

      tmp.replace( QFL("appSysName"), QString(utsn.sysname) );
      tmp.replace( QFL("appSysRelease"), QString(utsn.release) );
      tmp.replace( QFL("appMachineType"), QString(utsn.machine) );

      QStringList languageList = KGlobal::locale()->languageList();
      if ( languageList.count() )
      {
        int ind = languageList.indexOf( QLatin1String("C") );
        if( ind >= 0 )
        {
          if( languageList.contains( QLatin1String("en") ) )
            languageList.removeAt( ind );
          else
            languageList.value(ind) = QLatin1String("en");
        }
      }

      tmp.replace( QFL("appLanguage"), QString("%1").arg(languageList.join(", ")) );
      tmp.replace( QFL("appPlatform"), QFL("X11") );
    }

    // Ignore dups...
    if ( m_lstIdentity.contains(tmp) )
      continue;

    m_lstIdentity << tmp;

    tmp = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-SYSNAME")).arg(UA_PTOS("X-KDE-UA-SYSRELEASE"));
    if ( tmp.trimmed().isEmpty() )
      tmp = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-"
                    "NAME")).arg(UA_PTOS("X-KDE-UA-VERSION"));
    else
      tmp = QString("%1 %2 on %3").arg(UA_PTOS("X-KDE-UA-"
                    "NAME")).arg(UA_PTOS("X-KDE-UA-VERSION")).arg(tmp);

    m_lstAlias << tmp;
  }

  m_bIsDirty = false;
}

QString FakeUASProvider::aliasStr( const QString& name )
{
  int id = userAgentStringList().indexOf(name);
  if ( id == -1 )
    return QString();
  else
    return m_lstAlias[id];
}

QString FakeUASProvider::agentStr( const QString& name )
{
  int id = userAgentAliasList().indexOf(name);
  if ( id == -1 )
    return QString();
  else
    return m_lstIdentity[id];
}


QStringList FakeUASProvider::userAgentStringList()
{
  if ( m_bIsDirty )
  {
    loadFromDesktopFiles();
    if ( !m_providers.count() )
      return QStringList();
    parseDescription();
  }
  return m_lstIdentity;
}

QStringList FakeUASProvider::userAgentAliasList ()
{
  if ( m_bIsDirty )
  {
    loadFromDesktopFiles();
    if ( !m_providers.count() )
      return QStringList();
    parseDescription();
  }
  return m_lstAlias;
}

