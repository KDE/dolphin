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

#ifndef _konq_misc_h
#define _konq_misc_h

// This file can hold every global class for konqueror that used to pollute
// konq_main.cc

#include <kbookmark.h>
#include <krun.h>
#include <kparts/browserextension.h>
class KonqMainWindow;

class KonqBookmarkManager : public KBookmarkManager
{
public:
  KonqBookmarkManager( QString path ) : KBookmarkManager ( path ) {}
  ~KonqBookmarkManager() {}
  virtual void editBookmarks( const KURL & url );
};

class KonqMisc
{
public:
    /*
    private:
      static KonqFileManager *s_pSelf;
    public:
    KonqFileManager() {}
    ~KonqFileManager() {}

    static KonqFileManager *self()
    {
      if ( !s_pSelf )
      s_pSelf = new KonqFileManager();
      return s_pSelf;
     }
    */

    /**
     * Stop full-screen mode in all windows.
     */
    static void abortFullScreenMode();

    /**
     * Create a new window with a single view, showing @p url
     */
    static KonqMainWindow * createSimpleWindow( const KURL &url, const QString &frameName = QString::null );

    /**
     * Create a new window for @p url using @p args and the appropriate profile for this URL.
     */
    static KonqMainWindow * createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );

    /**
     * Create a new window from the profile defined by @p filename and @p path.
     * @param url an optionnal URL to open in this profile.
     */
    static KonqMainWindow * createBrowserWindowFromProfile( const QString &path,
                                                            const QString &filename,
                                                            const KURL &url = KURL(),
                                                            const KParts::URLArgs &args = KParts::URLArgs());

    /**
     * Applies the URI filters to @p url.
     * @p parent is used in case of a message box.
     */
    static QString konqFilteredURL( QWidget * parent, const QString &url );
};

#endif
