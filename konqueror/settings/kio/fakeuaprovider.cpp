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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <time.h>
#include <sys/utsname.h>

#include <qstringlist.h>

#include <kapp.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>

#include "fakeuaprovider.h"

#define UA_PTOS(x) (*it)->property(x).toString()
#define QFL(x) QString::fromLatin1(x)

FakeUASProvider::FakeUASProvider()
{
   m_bIsDirty = true;
}

FakeUASProvider::StatusCode FakeUASProvider::createNewUAProvider( const QString& uaStr )
{
  QStringList split;
  int pos = (uaStr).find("::");
  if ( pos == -1 )
  {
    pos = (uaStr).find(':');
    if ( pos != -1 )
    {
      split.append(uaStr.left(pos));
      split.append(uaStr.mid(pos+1));
    }
  }
  else
  {
    split = QStringList::split("::", uaStr);
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
/*
  //TODO: parse the UA string for the necessary info and
  // create a new desktop file out of it.

  bool hasSysName = uaStr.contains(QFL("appSysName"));
  bool hasSysRelease = uaStr.contains(QFL("appSysRelease"));
  bool hasDynamicEntry = (hasSysName || hasSysRelease ||
                          uaStr.contains(QFL("appName")) ||
                          uaStr.contains(QFL("appVersion")));


  QString dekstopfilename;
  if ( !verifyDesktopFilename(pv.desktopFilename) )
    return ALREADY_EXISTS;

  KConfig desktop( desktopFilename );
  desktop.setGroup("Desktop Entry");
  desktop.writeEntry("Name", QString("UADescription (%1 on %2 "
                     "%3)").arg(tag).arg(pv.appName).arg(pv.appVersion));
  desktop.writeEntry("Type", "Service");
  desktop.writeEntry("ServiceTypes", "UserAgentStrings");
  desktop.writeEntry("X-KDE-UA-TAG", tag);
  desktop.writeEntry("X-KDE-UA-FULL", uaStr);
  desktop.writeEntry("X-KDE-UA-NAME", appName);
  desktop.writeEntry("X-KDE-UA-VERSION", appVersion);
  if ( hasDynamicEntry )
  {
    desktop.writeEntry("X-KDE-UA-DYNAMIC-ENTRY", 1);
  }
  if ( !hasSysName )
    desktop.writeEntry("X-KDE-UA-SYSNAME", appSysName);
  if ( !hasSysRelease )
    desktop.writeEntry("X-KDE-UA-SYSRELEASE", appSysRelease);
  desktop.sync();
*/
  return SUCCEEDED;
}

bool FakeUASProvider::verifyDesktopFilename( QString& filename )
{
  QString path = locateLocal("services", "useragentstrings/");
  kdDebug() << "Path: " << path << endl;
  if ( !filename.isEmpty() && KStandardDirs::exists(path+filename) )
    return false;
  if ( filename.isEmpty() )
    filename = QString::number(time(0));
  filename = QString("%1%2.desktop").arg(path).arg(filename);
  kdDebug() << "New Filename: " << filename << endl;
  return true;
}

void FakeUASProvider::loadFromDesktopFiles()
{
  m_providers.clear();
  m_providers = KTrader::self()->query("UserAgentStrings");
}

void FakeUASProvider::parseDescription()
{
  QString tmp;
  KTrader::OfferList::ConstIterator it = m_providers.begin();
  for ( ; it != m_providers.end(); ++it )
  {
    QString tmp = UA_PTOS("X-KDE-UA-FULL");
    if ( (*it)->property("X-KDE-UA-DYNAMIC-ENTRY").toBool() )
    {
      int pos;
      struct utsname utsn;
      uname( &utsn );

      if ( (pos=tmp.find("appSysName")) != -1 )
        tmp.replace( pos, 10, QString(utsn.sysname) );
      if ( (pos=tmp.find("appSysRelease")) != -1 )
        tmp.replace( pos, 13, QString(utsn.release) );
      if ( (pos=tmp.find("appMachineType")) != -1 )
        tmp.replace( pos, 14, QString(utsn.machine) );

      QStringList languageList = KGlobal::locale()->languageList();
      if ( languageList.count() )
      {
        QStringList::Iterator it = languageList.find( QString::fromLatin1("C") );
        if( it != languageList.end() )
        {
          if( languageList.contains( QString::fromLatin1("en") ) > 0 )
            languageList.remove( it );
          else
            (*it) = QString::fromLatin1("en");
        }
      }
      if ( (pos=tmp.find("appLanguage")) != -1 )
        tmp.replace( pos, 11, QString("%1").arg(languageList.join(", ")) );
      if ( (pos=tmp.find("appPlatform")) != -1 )
        tmp.replace( pos, 11, QString::fromLatin1("X11") );
    }
    if ( !m_lstIdentity.contains(tmp) )
      m_lstIdentity << tmp;
    else
      continue; // Ignore dups!

    tmp = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-SYSNAME")).arg(UA_PTOS("X-KDE-UA-SYSRELEASE"));
    if ( tmp.stripWhiteSpace().isEmpty() )
      tmp = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-"
                    "NAME")).arg(UA_PTOS("X-KDE-UA-VERSION"));
    else
      tmp = QString("%1 %2 on %3").arg(UA_PTOS("X-KDE-UA-"
                    "NAME")).arg(UA_PTOS("X-KDE-UA-VERSION")).arg(tmp);
    m_lstAlias << tmp;
  }
  m_bIsDirty = false;
}

QString FakeUASProvider::aliasFor( const QString& name )
{
  int id = userAgentStringList().findIndex(name);
  if ( id == -1 )
    return QString::null;
  else
    return m_lstAlias[id];
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
