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
#ifndef __kbookmark_h
#define __kbookmark_h

#include <qstring.h>
#include <qobject.h>
#include <qdom.h>

class KBookmarkManager;

class KBookmark
{
    friend class KBookmarkGroup;
public:
    KBookmark( QDomElement elem ) : element(elem) {}

    /**
     * Whether the bookmark is a group or a normal bookmark
     */
    bool isGroup() const;

    bool isNull() const {return element.isNull();}

    /**
     * Text shown for the bookmark
     * If bigger than @p maxlen, the text is shortened by
     * replacing middle characters with "..." (see KStringHandler::csqueeze)
     * You should not have to change maxlen - this is mainly for keditbookmarks.
     */
    QString text( uint maxlen = 40 ) const;
    /**
     * URL contained by the bookmark
     */
    QString url() const;
    /**
     * @return the pixmap file for this bookmark
     * (i.e. the name of the icon)
     */
    QString icon() const;

    // TODO move up, move down, etc.

    KBookmarkGroup toGroup( KBookmarkManager * manager ) const;

protected:
    QDomElement element;
    // Note: you can't add new member variables here.
    // The KBookmarks are created on the fly, as wrappers
    // around internal QDomElements. Any additional information
    // has to be implemented as an attribute of the QDomElement.
};

/**
 * A group of bookmarks
 */
class KBookmarkGroup : public KBookmark
{
public:
    KBookmarkGroup( KBookmarkManager * manager, QDomElement elem ) : KBookmark(elem)
    {
        m_manager = manager;
    }
    bool operator == ( const KBookmarkGroup & g ) const { return element == g.element; }

    KBookmark first() const;
    KBookmark next( KBookmark & current ) const;

    KBookmarkManager * manager() const { return m_manager; }

    void createNewFolder( const QString & text );
    void addBookmark( const QString & text, const QString & url );

    bool isToolbarGroup() const;
    /**
     * @internal
     */
    QDomElement findToolbar() const;

    static KBookmarkGroup null() { return KBookmarkGroup(0,QDomElement()); }

private:
    KBookmarkManager * m_manager;
    // Note: you can't add new member variables here.
    // The KBookmarks are created on the fly, as wrappers
    // around internal QDomElements. Any additional information
    // has to be implemented as an attribute of the QDomElement.
};

/**
 * This class implements the reading/writing of bookmarks in XML.
 * The bookmarks file is defined this way :
 * <BOOKMARKS>
 *   <BOOKMARK URL="http://developer.kde.org">Developer Web Site</BOOKMARK>
 *   <GROUP>
 *     <BOOKMARK URL="http://www.kde.org">KDE Web Site</BOOKMARK>
 *     <GROUP>
 *       <TEXT>My own bookmarks</TEXT>
 *       <BOOKMARK URL="http://www.koffice.org">KOffice Web Site</BOOKMARK>
 *       <BOOKMARK URL="http://www.kdevelop.org">KDevelop Web Site</BOOKMARK>
 *     </GROUP>
 *   </GROUP>
 * </BOOKMARKS>
 */
class KBookmarkManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a bookmark manager with a path to the bookmarks.  By
     * default, it will use the KDE standard dirs to find and create the
     * correct location.  If you are using your own app-specific
     * bookmarks directory, you must instantiate this class with your
     * own path <em>before</em> KBookmarkManager::self() is every
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

    void parse();

    void save();

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
    KBookmarkGroup root();

    /**
     * This returns the root of the toolbar menu.
     * In the XML, this is the group with the attribute TOOLBAR=1
     *
     * @return the toolbar group
     */
    KBookmarkGroup toolbar();

    /**
     * @internal (for KBookmarkGroup)
     */
    void emitChanged( KBookmarkGroup & group );

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

public slots:
    void slotEditBookmarks();

signals:
    void changed( KBookmarkGroup & group );

protected:

    void importDesktopFiles();
    //void printGroup( QDomElement & group, int indent = 0 );
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
