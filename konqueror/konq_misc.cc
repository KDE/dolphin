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
#include <kstddirs.h>
#include <kglobal.h>
#include <kwin.h>
#include <kprotocolinfo.h>
#include <qdir.h>

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

// Terminates fullscreen-mode for any full-screen window on the current desktop
void KonqMisc::abortFullScreenMode()
{
  QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    int currentDesktop = KWin::currentDesktop();
    QListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
    {
      if ( it.current()->fullScreenMode() )
      {
	KWin::Info info = KWin::info( it.current()->winId() );
	if ( info.desktop == currentDesktop )
          it.current()->slotToggleFullScreen();
      }
    }
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
  kdDebug() << "KonqMisc::createNewWindow url=" << url.url() << endl;

  // For HTTP or html files, use the web browsing profile, otherwise use filemanager profile
  QString profileName = (!(KProtocolInfo::supportsListing(url)) ||
                        url.path().right(5) == ".html" ||
			url.path().right(4) == ".htm" )
          ? "webbrowsing" : "filemanagement";

  QString profile = locate( "data", QString::fromLatin1("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile( profile, profileName, url, args );
}


KonqMainWindow * KonqMisc::createBrowserWindowFromProfile( const QString &path, const QString &filename, const KURL &url, const KParts::URLArgs &args )
{
  kdDebug(1202) << "void KonqMisc::createBrowserWindowFromProfile() " << endl;
  kdDebug(1202) << "path=" << path << ",filename=" << filename << ",url=" << url.prettyURL() << endl;
  abortFullScreenMode();

  KonqMainWindow * mainWindow;
  if ( path.isEmpty() )
  {
      // The profile doesn't exit -> creating a simple window
      mainWindow = createSimpleWindow( url, args.frameName );
  }
  else
  {
      mainWindow = new KonqMainWindow( KURL(), false );
      //FIXME: obey args (like passing post-data (to KRun), etc.)
      KonqOpenURLRequest req;
      req.args = args;
      mainWindow->viewManager()->loadViewProfile( path, filename, url, req );
  }
  mainWindow->setInitialFrameName( args.frameName );
  mainWindow->show();
  return mainWindow;
}

QString KonqMisc::konqFilteredURL( QWidget* parent, const QString& _url, const QString& _path )
{
  if ( !_url.startsWith( "about:" ) ) // Don't filter "about:" URLs
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
  }
  else if ( _url.startsWith( "about:" ) && _url != "about:blank" ) {
    // We can't use "about:" as it is, KURL doesn't parse it.
    return "about:konqueror";
  }
  return _url;  // return the original url if it cannot be filtered.
}

