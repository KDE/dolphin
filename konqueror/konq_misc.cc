/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_misc.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurifilter.h>
#include <klocale.h>
#include <qdir.h>

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

void KonqMisc::abortFullScreenMode()
{
  QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    QListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
      if ( it.current()->fullScreenMode() )
        it.current()->slotToggleFullScreen();
  }
}

KonqMainWindow * KonqMisc::createSimpleWindow( const KURL & _url, const QString &frameName )
{
  abortFullScreenMode();

  // If _url is 0L, open $HOME [this doesn't happen anymore]
  KURL url = !_url.isEmpty() ? _url : KURL(QDir::homeDirPath().prepend( "file:" ));

  KonqMainWindow *win = new KonqMainWindow( url );
  win->setInitialFrameName( frameName );
  win->show();

  return win;
}

KonqMainWindow * KonqMisc::createNewWindow( const KURL &url, const KParts::URLArgs &args )
{
  abortFullScreenMode();

  // For HTTP, use the web browsing profile, otherwise use filemanager profile
  // TODO: maybe add a list of protocols in the profile...
  QString profileName = url.protocol().startsWith("http") ? "webbrowsing" : "filemanagement";

  QString profile = locate( "data", QString::fromLatin1("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile( profile, profileName, url, args );
}


KonqMainWindow * KonqMisc::createBrowserWindowFromProfile( const QString &path, const QString &filename, const KURL &url, const KParts::URLArgs &args )
{
  kdDebug(1202) << "void KonqMisc::createBrowserWindowFromProfile() " << endl;
  kdDebug(1202) << "path=" << path << ",filename=" << filename << ",url=" << url.prettyURL() << endl;

  KonqMainWindow * mainWindow;
  if ( path.isEmpty() )
  {
      // The profile doesn't exit -> creating a simple window
      mainWindow = createSimpleWindow( url, args.frameName );
  }
  else
  {
      mainWindow = new KonqMainWindow( QString::null, false );
      //FIXME: obey args (like passing post-data (to KRun), etc.)
      mainWindow->viewManager()->loadViewProfile( path, filename, url );
  }
  if (mainWindow->currentView())
      mainWindow->enableAllActions( true );
  else
      mainWindow->disableActionsNoView();
  mainWindow->setInitialFrameName( args.frameName );
  mainWindow->show();
  return mainWindow;
}

QString KonqMisc::konqFilteredURL( QWidget* parent, const QString& _url, const QString& _path )
{
  KURIFilterData data = _url;

  if( !_path.isEmpty() )
    data.setAbsolutePath(_path);

  if( KURIFilter::self()->filterURI( data ) )
  {
    if( data.uriType() == KURIFilterData::ERROR && !data.errorMsg().isEmpty() )
      KMessageBox::sorry( parent, i18n( data.errorMsg().utf8() ) );
    else
      return data.uri().url();
  }
  return _url;  // return the original url if it cannot be filtered.
}

/**********************************************
 *
 * KonqBookmarkManager
 *
 **********************************************/

void KonqBookmarkManager::editBookmarks( const KURL & _url )
{
  KonqMisc::createSimpleWindow( _url );
}

