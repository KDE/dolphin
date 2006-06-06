/*
   Copyright (C) 2001 Dawit Alemayehu <adawit@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstaticdeleter.h>
#include <kio/ioslave_defaults.h>
#include <dbus/qdbus.h>

#include "ksaveioconfig.h"

class KSaveIOConfigPrivate
{
public:
  KSaveIOConfigPrivate ();
  ~KSaveIOConfigPrivate ();

  KConfig* config;
  KConfig* http_config;
};

static KSaveIOConfigPrivate *ksiocpref = 0;
static KStaticDeleter<KSaveIOConfigPrivate> ksiocp;

KSaveIOConfigPrivate::KSaveIOConfigPrivate (): config(0), http_config(0)
{
  ksiocp.setObject (ksiocpref, this);
}

KSaveIOConfigPrivate::~KSaveIOConfigPrivate ()
{
  delete config;
}

KSaveIOConfigPrivate* KSaveIOConfig::d = 0;

KConfig* KSaveIOConfig::config()
{
  if (!d)
     d = new KSaveIOConfigPrivate;

  if (!d->config)
     d->config = new KConfig("kioslaverc", false, false);

  return d->config;
}

KConfig* KSaveIOConfig::http_config()
{
  if (!d)
     d = new KSaveIOConfigPrivate;

  if (!d->http_config)
     d->http_config = new KConfig("kio_httprc", false, false);

  return d->http_config;
}

void KSaveIOConfig::reparseConfiguration ()
{
  delete d->config;
  d->config = 0;
}

void KSaveIOConfig::setReadTimeout( int _timeout )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry("ReadTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
}

void KSaveIOConfig::setConnectTimeout( int _timeout )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry("ConnectTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
}

void KSaveIOConfig::setProxyConnectTimeout( int _timeout )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry("ProxyConnectTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
}

void KSaveIOConfig::setResponseTimeout( int _timeout )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry("ResponseTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
}


void KSaveIOConfig::setMarkPartial( bool _mode )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry( "MarkPartial", _mode );
  cfg->sync();
}

void KSaveIOConfig::setMinimumKeepSize( int _size )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry( "MinimumKeepSize", _size );
  cfg->sync();
}

void KSaveIOConfig::setAutoResume( bool _mode )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry( "AutoResume", _mode );
  cfg->sync();
}

void KSaveIOConfig::setUseCache( bool _mode )
{
  KConfig* cfg = http_config ();
  cfg->writeEntry( "UseCache", _mode );
  cfg->sync();
}

void KSaveIOConfig::setMaxCacheSize( int cache_size )
{
  KConfig* cfg = http_config ();
  cfg->writeEntry( "MaxCacheSize", cache_size );
  cfg->sync();
}

void KSaveIOConfig::setCacheControl(KIO::CacheControl policy)
{
  KConfig* cfg = http_config ();
  QString tmp = KIO::getCacheControlString(policy);
  cfg->writeEntry("cache", tmp);
  cfg->sync();
}

void KSaveIOConfig::setMaxCacheAge( int cache_age )
{
  KConfig* cfg = http_config ();
  cfg->writeEntry( "MaxCacheAge", cache_age );
  cfg->sync();
}

void KSaveIOConfig::setUseReverseProxy( bool mode )
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry("ReversedException", mode);
  cfg->sync();
}

void KSaveIOConfig::setProxyType(KProtocolManager::ProxyType type)
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "ProxyType", static_cast<int>(type) );
  cfg->sync();
}

void KSaveIOConfig::setProxyAuthMode(KProtocolManager::ProxyAuthMode mode)
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "AuthMode", static_cast<int>(mode) );
  cfg->sync();
}

void KSaveIOConfig::setNoProxyFor( const QString& _noproxy )
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "NoProxyFor", _noproxy );
  cfg->sync();
}

void KSaveIOConfig::setProxyFor( const QString& protocol,
                                 const QString& _proxy )
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( protocol.toLower() + "Proxy", _proxy );
  cfg->sync();
}

void KSaveIOConfig::setProxyConfigScript( const QString& _url )
{
  KConfig* cfg = config ();
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "Proxy Config Script", _url );
  cfg->sync();
}

void KSaveIOConfig::setPersistentProxyConnection( bool enable )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry( "PersistentProxyConnection", enable );
  cfg->sync();
}

void KSaveIOConfig::setPersistentConnections( bool enable )
{
  KConfig* cfg = config ();
  cfg->setGroup( QString() );
  cfg->writeEntry( "PersistentConnections", enable );
  cfg->sync();
}

void KSaveIOConfig::updateRunningIOSlaves (QWidget *parent)
{
  // Inform all running io-slaves about the changes...
  // if we cannot update, ioslaves inform the end user...
#warning "kde4: port it to new dbus call"
#if 0
  if (!DCOPRef("*", "KIO::Scheduler").send("reparseSlaveConfiguration", QString()))
  {
    QString caption = i18n("Update Failed");
    QString message = i18n("You have to restart the running applications "
                           "for these changes to take effect.");
    KMessageBox::information (parent, message, caption);
    return;
  }
#endif
}

void KSaveIOConfig::updateProxyScout( QWidget * parent )
{
  // Inform the proxyscout kded module about changes
  // if we cannot update, ioslaves inform the end user...
    QDBusInterfacePtr kded("org.kde.kded", "/Proxyscout", "org.kde.kded.ProxyScout");
    QDBusReply<void> reply = kded->call( "reset" );
  if (!reply.isSuccess())
  {
    QString caption = i18n("Update Failed");
    QString message = i18n("You have to restart KDE "
                           "for these changes to take effect.");
    KMessageBox::information (parent, message, caption);
    return;
  }
}

