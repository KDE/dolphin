/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "KonquerorIface.h"
#include "konq_misc.h"
#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"
#include <konq_settings.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <QFile>
//Added by qt3to4:
#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#endif
#include "konq_settingsxt.h"

// these DCOP calls come from outside, so any windows created by these
// calls would have old user timestamps (for KWin's no-focus-stealing),
// it's better to reset the timestamp and rely on other means
// of detecting the time when the user action that triggered all this
// happened
// TODO a valid timestamp should be passed in the DCOP calls that
// are not for user scripting

KonquerorIface::KonquerorIface()
 : DCOPObject( "KonquerorIface" )
{
}

KonquerorIface::~KonquerorIface()
{
}

DCOPRef KonquerorIface::openBrowserWindow( const QString &url )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createSimpleWindow( KUrl(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::openBrowserWindowASN( const QString &url, const DCOPCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return openBrowserWindow( url );
}

DCOPRef KonquerorIface::createNewWindow( const QString &url )
{
    return createNewWindow( url, QString(), false );
}

DCOPRef KonquerorIface::createNewWindowASN( const QString &url, const DCOPCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, QString(), tempFile );
}

DCOPRef KonquerorIface::createNewWindowWithSelection( const QString &url, QStringList filesToSelect )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createNewWindow( KUrl(url), KParts::URLArgs(), false, filesToSelect );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowWithSelectionASN( const QString &url, QStringList filesToSelect, const DCOPCString &startup_id )
{
    kapp->setStartupId( startup_id );
    return createNewWindowWithSelection( url, filesToSelect );
}

DCOPRef KonquerorIface::createNewWindow( const QString &url, const QString &mimetype, bool tempFile )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KParts::URLArgs args;
    args.serviceType = mimetype;
    // Filter the URL, so that "kfmclient openURL gg:foo" works also when konq is already running
    KUrl finalURL = KonqMisc::konqFilteredURL( 0, url );
    KonqMainWindow *res = KonqMisc::createNewWindow( finalURL, args, false, QStringList(), tempFile );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowASN( const QString &url, const QString &mimetype,
    const DCOPCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, mimetype, tempFile );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const QString &path )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    kDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( const QString &path ) " << endl;
    kDebug(1202) << path << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, QString() );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const DCOPCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const QString & path, const QString &filename )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    kDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( path, filename ) " << endl;
    kDebug(1202) << path << "," << filename << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const QString &filename,
    const DCOPCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path, filename );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const QString & path, const QString &filename, const QString &url )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KUrl(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url,
    const DCOPCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url, const QString &mimetype )
{
#ifdef Q_WS_X11
    QX11Info::setAppUserTime( 0 );
#endif
    KParts::URLArgs args;
    args.serviceType = mimetype;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KUrl(url), args );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url, const QString &mimetype,
    const DCOPCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url, mimetype );
}


void KonquerorIface::reparseConfiguration()
{
  KGlobal::config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();

  QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    foreach ( KonqMainWindow* window, *mainWindows )
        window->reparseConfiguration();
  }
}

void KonquerorIface::updateProfileList()
{
  QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
  if ( !mainWindows )
    return;

  foreach ( KonqMainWindow* window, *mainWindows )
    window->viewManager()->profileListDirty( false );
}

QString KonquerorIface::crashLogFile()
{
    return KonqMainWindow::s_crashlog_file->objectName();
}

QList<DCOPRef> KonquerorIface::getWindows()
{
    QList<DCOPRef> lst;
    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
      foreach ( KonqMainWindow* window, *mainWindows )
        lst.append( DCOPRef( kapp->dcopClient()->appId(), window->dcopObject()->objId() ) );
    }
    return lst;
}

void KonquerorIface::addToCombo( QString url, DCOPCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, objId );
}

void KonquerorIface::removeFromCombo( QString url, DCOPCString objId )
{
  KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, objId );
}

void KonquerorIface::comboCleared( DCOPCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear,
				 QString(), objId );
}

bool KonquerorIface::processCanBeReused( int screen )
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

void KonquerorIface::terminatePreloaded()
{
    if( KonqMainWindow::isPreloaded())
        kapp->exit();
}
