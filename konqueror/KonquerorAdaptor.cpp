/* This file is part of the KDE project
   Copyright 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright 2000, 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonquerorAdaptor.h"
#include "konq_misc.h"
#include "KonqMainWindowAdaptor.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"
#include "konq_settingsxt.h"
#include <konq_settings.h>

#include <kapplication.h>
#include <kdebug.h>

#include <QFile>
#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#endif

// these DBus calls come from outside, so any windows created by these
// calls would have old user timestamps (for KWin's no-focus-stealing),
// it's better to reset the timestamp and rely on other means
// of detecting the time when the user action that triggered all this
// happened
// TODO a valid timestamp should be passed in the DBus calls that
// are not for user scripting

KonquerorAdaptor::KonquerorAdaptor()
 : QDBusAbstractAdaptor( kapp )
{
}

KonquerorAdaptor::~KonquerorAdaptor()
{
}

QDBusObjectPath KonquerorAdaptor::openBrowserWindow( const QString& url, const QByteArray& startup_id )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createSimpleWindow( KUrl(url) );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

QDBusObjectPath KonquerorAdaptor::createNewWindow( const QString& url, const QString& mimetype, const QByteArray& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KParts::URLArgs args;
    args.serviceType = mimetype;
    // Filter the URL, so that "kfmclient openURL gg:foo" works also when konq is already running
    KUrl finalURL = KonqMisc::konqFilteredURL( 0, url );
    KonqMainWindow *res = KonqMisc::createNewWindow( finalURL, args, false, QStringList(), tempFile );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

QDBusObjectPath KonquerorAdaptor::createNewWindowWithSelection( const QString& url, const QStringList& filesToSelect, const QByteArray& startup_id )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createNewWindow( KUrl(url), KParts::URLArgs(), false, filesToSelect );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

QDBusObjectPath KonquerorAdaptor::createBrowserWindowFromProfile( const QString& path, const QString& filename, const QByteArray& startup_id )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    kDebug(1202) << "void KonquerorAdaptor::createBrowserWindowFromProfile( path, filename ) " << endl;
    kDebug(1202) << path << "," << filename << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

QDBusObjectPath KonquerorAdaptor::createBrowserWindowFromProfileAndUrl( const QString& path, const QString& filename, const QString& url, const QByteArray& startup_id )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KUrl(url) );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

QDBusObjectPath KonquerorAdaptor::createBrowserWindowFromProfileUrlAndMimeType( const QString& path, const QString& filename, const QString& url, const QString& mimetype, const QByteArray& startup_id )
{
    kapp->setStartupId( startup_id );
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KParts::URLArgs args;
    args.serviceType = mimetype;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KUrl(url), args );
    if ( !res )
        return QDBusObjectPath();
    return QDBusObjectPath( '/' + res->objectName() ); // this is what KMainWindow sets as the dbus object path
}

void KonquerorAdaptor::updateProfileList()
{
  QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
  if ( !mainWindows )
    return;

  foreach ( KonqMainWindow* window, *mainWindows )
    window->viewManager()->profileListDirty( false );
}

QString KonquerorAdaptor::crashLogFile()
{
    return KonqMainWindow::s_crashlog_file->objectName();
}

QList<QDBusObjectPath> KonquerorAdaptor::getWindows()
{
    QList<QDBusObjectPath> lst;
    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
      foreach ( KonqMainWindow* window, *mainWindows )
        lst.append( QDBusObjectPath( '/' + window->objectName() ) );
    }
    return lst;
}

void KonquerorAdaptor::addToCombo( const QString& url, const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, msg.sender() );
}

void KonquerorAdaptor::removeFromCombo( const QString& url, const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, msg.sender() );
}

void KonquerorAdaptor::comboCleared( const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear, QString(), msg.sender() );
}

bool KonquerorAdaptor::processCanBeReused( int screen )
{
#ifdef Q_WS_X11
	QX11Info info;
    if( info.screen() != screen )
        return false; // this instance run on different screen, and Qt apps can't migrate
#endif
    if( KonqMainWindow::isPreloaded())
        return false; // will be handled by preloading related code instead
    QList<KonqMainWindow*>* windows = KonqMainWindow::mainWindowList();
    if( windows == NULL )
        return true;
    QStringList allowed_parts = KonqSettings::safeParts();
    bool all_parts_allowed = false;

    if( allowed_parts.count() == 1 && allowed_parts.first() == QLatin1String( "SAFE" ))
    {
        allowed_parts.clear();
        // is duplicated in client/kfmclient.cc
        allowed_parts << QLatin1String( "konq_iconview.desktop" )
                      << QLatin1String( "konq_multicolumnview.desktop" )
                      << QLatin1String( "konq_sidebartng.desktop" )
                      << QLatin1String( "konq_infolistview.desktop" )
                      << QLatin1String( "konq_treeview.desktop" )
                      << QLatin1String( "konq_detailedlistview.desktop" );
    }
    else if( allowed_parts.count() == 1 && allowed_parts.first() == QLatin1String( "ALL" ))
    {
        allowed_parts.clear();
        all_parts_allowed = true;
    }
    if( all_parts_allowed )
        return true;
    foreach ( KonqMainWindow* window, *windows )
    {
        kDebug(1202) << "processCanBeReused: count=" << window->viewCount() << endl;
        const KonqMainWindow::MapViews& views = window->viewMap();
        foreach ( KonqView* view, views )
        {
            kDebug(1202) << "processCanBeReused: part=" << view->service()->desktopEntryPath() << ", URL=" << view->url().prettyUrl() << endl;
            if( !allowed_parts.contains( view->service()->desktopEntryPath()))
                return false;
        }
    }
    return true;
}

void KonquerorAdaptor::terminatePreloaded()
{
    if( KonqMainWindow::isPreloaded())
        kapp->exit();
}

#include "KonquerorAdaptor.moc"
