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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _konq_misc_h
#define _konq_misc_h

// This file can hold every global class for konqueror that used to pollute
// konq_main.cc

#include <krun.h>
#include <kparts/browserextension.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
class KonqMainWindow;
class KonqView;

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
    static KonqMainWindow * createSimpleWindow( const KUrl &url, const QString &frameName = QString() );

    /**
     * Create a new window with a single view, showing @p url, using @p args
     */
    static KonqMainWindow * createSimpleWindow( const KUrl &url, const KParts::URLArgs &args, 
						bool tempFile = false);

    /**
     * Create a new window for @p url using @p args and the appropriate profile for this URL.
     * @param forbidUseHTML internal. True when called by "Find Files"
     * @param openURL If it is false, no url is openend in the new window. The url is used to guess the profile
     */
    static KonqMainWindow * createNewWindow( const KUrl &url,
                                             const KParts::URLArgs &args = KParts::URLArgs(),
                                             bool forbidUseHTML = false,
                                             QStringList filesToSelect = QStringList(),
                                             bool tempFile = false,
					     bool openURL = true);

    /**
     * Create a new window from the profile defined by @p filename and @p path.
     * @param url an optional URL to open in this profile.
     * @param forbidUseHTML internal. True when called by "Find Files"
     * @param openURL If false no url is opened
     */
    static KonqMainWindow * createBrowserWindowFromProfile( const QString &path,
                                                            const QString &filename,
                                                            const KUrl &url = KUrl(),
                                                            const KParts::URLArgs &args = KParts::URLArgs(),
                                                            bool forbidUseHTML = false,
                                                            const QStringList& filesToSelect = QStringList(),
                                                            bool tempFile = false,
							    bool openURL = true);

    /**
     * Creates a new window from the history of a view, copies the history
     * @param view the History is copied from this view
     * @param steps Restore currentPos() + steps
     */
    static KonqMainWindow * newWindowFromHistory( KonqView* view, int steps );

    /**
     * Applies the URI filters to @p url.
     *
     * @p parent is used in case of a message box.
     * @p _url to be filtered.
     * @p _path the absolute path to append to the url before filtering it.
     */
    static QString konqFilteredURL( QWidget* /*parent*/, const QString& /*_url*/, const QString& _path = QString() );

};

#include <QLabel>

class KonqDraggableLabel : public QLabel
{
    Q_OBJECT
public:
    KonqDraggableLabel( KonqMainWindow * mw, const QString & text );

protected:
    void mousePressEvent( QMouseEvent * ev );
    void mouseMoveEvent( QMouseEvent * ev );
    void mouseReleaseEvent( QMouseEvent * );
    void dragEnterEvent( QDragEnterEvent *ev );
    void dropEvent( QDropEvent* ev );

private Q_SLOTS:
    void delayedOpenURL();

private:
    QPoint startDragPos;
    bool validDrag;
    KonqMainWindow * m_mw;
    KUrl::List _savedLst;
};

#endif
