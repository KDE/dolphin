/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __kbookmarkmanager_h
#define __kbookmarkmanager_h

#include <qstring.h>
#include <qobject.h>
#include <qdom.h>
#include <dcopobject.h>
#include "kbookmark.h"

/**
 * This class implements the reading/writing of bookmarks in XML.
 * The bookmarks file is read and written using the XBEL standard
 * (http://pyxml.sourceforge.net/topics/xbel/)
 *
 * A sample file looks like this :
 * <xbel>
 *   <bookmark href="http://developer.kde.org"><title>Developer Web Site</title></bookmark>
 *   <folder folded="no">
 *     <title>Title of this folder</title>
 *     <bookmark icon="kde" href="http://www.kde.org"><title>KDE Web Site</title></bookmark>
 *     <folder toolbar="yes">
 *       <title>My own bookmarks</title>
 *       <bookmark href="http://www.koffice.org"><title>KOffice Web Site</title></bookmark>
 *       <separator/>
 *       <bookmark href="http://www.kdevelop.org"><title>KDevelop Web Site</title></bookmark>
 *     </folder>
 *   </folder>
 * </xbel>
 */
class KBookmarkManager : public QObject, public DCOPObject
{
    Q_OBJECT
    K_DCOP
public:
    /**
     * Creates a bookmark manager with a path to the bookmarks.  By
     * default, it will use the KDE standard dirs to find and create the
     * correct location.  If you are using your own app-specific
     * bookmarks directory, you must instantiate this class with your
     * own path <em>before</em> KBookmarkManager::self() is ever
     * called.
     *
     * @param bookmarksFile full path to the bookmarks file,
     * defaults to ~/.kde/share/apps/konqueror/bookmarks.xml
     *
     * @param bImportDesktopFiles if true, and if the bookmarksFile
     * doesn't already exist, import bookmarks from desktop files
     */
    KBookmarkManager( const QString & bookmarksFile = QString::null,
                         bool bImportDesktopFiles = true );

    ~KBookmarkManager();

    /**
     * Save the bookmarks to the XML file on disk.
     * @return true if saving was successful
     */
    bool save();

    /**
     * This will return the path that this manager is using to read
     * the bookmarks.
     * @internal
     * @return the path containing the bookmarks
     */
    QString path() { return m_bookmarksFile; }

    /**
     * This will return the root bookmark.  It is used to iterate
     * through the bookmarks manually.  It is mostly used internally.
     *
     * @return the root (top-level) bookmark
     */
    KBookmarkGroup root() const;

    /**
     * This returns the root of the toolbar menu.
     * In the XML, this is the group with the attribute TOOLBAR=1
     *
     * @return the toolbar group
     */
    KBookmarkGroup toolbar();

    /**
     * @return the bookmark designated by @p address
     * @see KBookmark::address
     */
    KBookmark findByAddress( const QString & address );

    /**
     * @internal (for KBookmarkGroup)
     */
    void emitChanged( KBookmarkGroup & group );

    /**
     * @return true if the NS bookmarks should be dynamically shown
     * in the toplevel kactionmenu
     */
    bool showNSBookmarks() const;

    /**
     * This static function will return an instance of the
     * KBookmarkManager.  If you do not instantiate this class either
     * natively or in a derived class, then it will return an object
     * with the default behaviors.  If you wish to use different
     * behaviors, you <em>must</em> derive your own class and
     * instantiate it before this method is ever called.
     *
     * @return a pointer to an instance of the KBookmarkManager.
     */
    static KBookmarkManager* self();

    /**
     * @internal
     */
    const QDomDocument & internalDocument() const { return m_doc; }

public slots:
    void slotEditBookmarks();

public:
k_dcop:
    /**
     * Reparse the whole bookmarks file and notify about the change
     * (Called by the bookmark editor)
     */
    ASYNC notifyCompleteChange( QString caller );

    /**
     * Emit the changed signal for the group whose address is given
     * @see KBookmark::address()
     * Called by the instance of konqueror that saved the file after
     * a small change (new bookmark or new folder).
     */
    ASYNC notifyChanged( QString groupAddress );

signals:
    /**
     * Signals that the group with the address @p groupAddress (e.g. "/4/5")
     * has been modified by the caller @p caller.
     */
    void changed( const QString & groupAddress, const QString & caller );

protected:
    void parse();
    void importDesktopFiles();
    void convertToXBEL( QDomElement & group );
    void convertAttribute( QDomElement elem, const QString & oldName, const QString & newName );
    QString m_bookmarksFile;
    QDomDocument m_doc;
    static KBookmarkManager* s_pSelf;
};

/**
 * The @ref KBookmarkMenu and @ref KBookmarkBar classes gives the user
 * the ability to either edit bookmarks or add their own.  In the
 * first case, the app may want to open the bookmark in a special way.
 * In the second case, the app <em>must</em> supply the name and the
 * URL for the bookmark.
 *
 * This class gives the app this callback-like ability.
 *
 * If your app does not give the user the ability to add bookmarks and
 * you don't mind using the default bookmark editor to edit your
 * bookmarks, then you don't need to overload this class at all.
 * Rather, just use something like:
 *
 * <CODE>
 * bookmarks = new KBookmarkMenu(new KBookmarkOwner(), ...)
 * </CODE>
 *
 * If you wish to use your own editor or allow the user to add
 * bookmarks, you must overload this class.
 */
class KBookmarkOwner
{
public:
  /**
   * This function is called if the user selects a bookmark.  It will
   * open up the bookmark in a default fashion unless you override it.
   */
  virtual void openBookmarkURL(const QString& _url);

  /**
   * This function is called whenever the user wants to add the
   * current page to the bookmarks list.  The title will become the
   * "name" of the bookmark.  You must overload this function if you
   * wish to give your users the ability to add bookmarks.
   *
   * @return the title of the current page.
   */
  virtual QString currentTitle() const { return QString::null; }

  /**
   * This function is called whenever the user wants to add the
   * current page to the bookmarks list.  The URL will become the URL
   * of the bookmark.  You must overload this function if you wish to
   * give your users the ability to add bookmarks.
   *
   * @return the URL of the current page.
   */
  virtual QString currentURL() const { return QString::null; }
};

#endif
