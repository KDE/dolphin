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
 * KonqFileManager
 *
 **********************************************/

KonqFileManager *KonqFileManager::s_pSelf = 0;

bool KonqFileManager::openFileManagerWindow( const KURL & _url )
{
  return openFileManagerWindow( _url, QString::null );
}

bool KonqFileManager::openFileManagerWindow( const KURL & _url, const QString &name )
{
  QList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    QListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
      if ( it.current()->fullScreenMode() )
      {
        it.current()->slotToggleFullScreen( false );
        static_cast<KToggleAction *>( it.current()->actionCollection()->action( "fullscreen" ) )->setChecked( false );
      }
  }

  // If _url is 0L, open $HOME [this doesn't happen anymore]
  KURL url = !_url.isEmpty() ? _url : KURL(QDir::homeDirPath().prepend( "file:" ));

  KonqMainWindow *win = new KonqMainWindow( url );
  win->setInitialFrameName( name );
  win->show();

  return true; // why would it fail ? :)
}

KonqMainWindow * KonqFileManager::createBrowserWindowFromProfile( const QString &filename, const QString &url )
{
  kdDebug(1202) << "void KonqFileManager::createBrowserWindowFromProfile() " << endl;
  kdDebug(1202) << filename << "," << url << endl;

  KonqMainWindow *mainWindow = new KonqMainWindow( QString::null, false );
  mainWindow->viewManager()->loadViewProfile( filename, KURL(url) );
  mainWindow->enableAllActions( true );
  mainWindow->show();
  return mainWindow;
}


QString konqFilteredURL( QWidget * parent, const QString &_url )
{
  KURIFilterData data = _url;
  KURIFilter::self()->filterURI( data );
  if( data.hasBeenFiltered() )
  {
    KURIFilterData::URITypes type = data.uriType();
    if( type == KURIFilterData::UNKNOWN )
    {
      KMessageBox::sorry( parent, i18n( "The url \"%1\" is of unknown type" ).arg( _url ) );
      return QString::null;  // should never happen unless the search filter is unavailable.
    }
    else if( type == KURIFilterData::ERROR )
    {
      KMessageBox::sorry( parent, i18n( data.errorMsg().utf8() ) );
      return QString::null;
    }
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
  KonqFileManager::self()->openFileManagerWindow( _url );
}

