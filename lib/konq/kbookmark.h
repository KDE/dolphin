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
#include <kurl.h>
#include <qdom.h>

class KBookmarkManager;

class KBookmark
{
    friend class KBookmarkGroup;
public:
    KBookmark( QDomElement elem ) : element(elem) {}

    static KBookmark standaloneBookmark( const QString & text, const KURL & url, const QString & icon = QString::null );

    /**
     * Whether the bookmark is a group or a normal bookmark
     */
    bool isGroup() const;

    /**
     * Whether the bookmark is a separator
     */
    bool isSeparator() const;

    /**
     * @return true if this is a null bookmark. This will never
     * be the case for a real bookmark (in a menu), but it's used
     * for instance as the end condition for KBookmarkGroup::next()
     */
    bool isNull() const {return element.isNull();}

    /**
     * Text shown for the bookmark
     * If bigger than 40, the text is shortened by
     * replacing middle characters with "..." (see KStringHandler::csqueeze)
     */
    QString text() const;
    /**
     * Text shown for the bookmark, not truncated.
     * You should not use this - this is mainly for keditbookmarks.
     */
    QString fullText() const;
    /**
     * URL contained by the bookmark
     */
    KURL url() const;
    /**
     * @return the pixmap file for this bookmark
     * (i.e. the name of the icon)
     */
    QString icon() const;

    /**
     * @return the group containing this bookmark
     */
    KBookmarkGroup parentGroup() const;

    /**
     * Convert this to a group - do this only if
     * isGroup() returns true.
     */
    KBookmarkGroup toGroup() const;

    /**
     * Return the "address" of this bookmark in the whole tree.
     * This is used when telling other processes about a change
     * in a given bookmark. The encoding of the address is "/4/2", for
     * instance, to design the 2nd child inside the 4th child of the root bk.
     */
    QString address() const;

    // Hard to decide. Good design would imply that each bookmark
    // knows about its manager, so that there can be several managers.
    // But if we say there is only one manager (i.e. set of bookmarks)
    // per application, then KBookmarkManager::self() is much easier.
    //KBookmarkManager * manager() const { return m_manager; }

    /**
     * @internal for KEditBookmarks
     */
    QDomElement internalElement() const { return element; }

    // Utility functions (internal)

    /**
     * @return address of parent
     */
    static QString parentAddress( const QString & address )
    { return address.left( address.findRev('/') ); }

    /**
     * @return position in parent (e.g. /4/5/2 -> 2)
     */
    static uint positionInParent( const QString & address )
    { return address.mid( address.findRev('/') + 1 ).toInt(); }

    /**
     * @return address of previous sibling (e.g. /4/5/2 -> /4/5/1)
     * Returns QString::null for a first child
     */
    static QString previousAddress( const QString & address )
    {
        uint pp = positionInParent(address);
        return pp>0 ? parentAddress(address) + '/' + QString::number(pp-1) : QString::null;
    }

    /**
     * @return address of next sibling (e.g. /4/5/2 -> /4/5/3)
     * This doesn't check whether it actually exists
     */
    static QString nextAddress( const QString & address )
    { return parentAddress(address) + '/' + QString::number(positionInParent(address)+1); }

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
    /**
     * Create an invalid group. This is mostly for use in QValueList,
     * and other places where we need a null group.
     * Also used as a parent for a bookmark that doesn't have one
     * (e.g. Netscape bookmarks)
     */
    KBookmarkGroup();

    /**
     * Create a bookmark group as specified by the given element
     */
    KBookmarkGroup( QDomElement elem );

    /**
     * Much like KBookmark::address, but caches the
     * address into m_address.
     */
    QString groupAddress() const;

    /**
     * @return true if the bookmark folder is opened in the bookmark editor
     */
    bool isOpen() const;

    /**
     * Return the first child bookmark of this group
     */
    KBookmark first() const;
    /**
     * Return the prevous sibling of a child bookmark of this group
     * @param current has to be one of our child bookmarks.
     */
    KBookmark previous( const KBookmark & current ) const;
    /**
     * Return the next sibling of a child bookmark of this group
     * @param current has to be one of our child bookmarks.
     */
    KBookmark next( const KBookmark & current ) const;

    /**
     * Create a new bookmark folder, as the last child of this group
     * @p text for the folder. If empty, the user will be queried for it.
     */
    KBookmarkGroup createNewFolder( const QString & text = QString::null );
    /**
     * Create a new bookmark separator
     */
    KBookmark createNewSeparator();
    /**
     * Create a new bookmark, as the last child of this group
     * Don't forget to use KBookmarkManager::self()->emitChanged( parentBookmark );
     * if this bookmark was added interactively.
     */
    KBookmark addBookmark( const QString & text, const KURL & url, const QString & icon = QString::null );

    /**
     * Moves @p item after @p after (which should be a child of ours).
     * If item is null, @p item is moved as the first child.
     */
    bool moveItem( const KBookmark & item, const KBookmark & after );

    /**
     * Delete a bookmark - it has to be one of our children !
     */
    void deleteBookmark( KBookmark bk );

    /**
     * @return true if this is the toolbar group
     */
    bool isToolbarGroup() const;
    /**
     * @internal
     */
    QDomElement findToolbar() const;

protected:
    QDomElement nextKnownTag( QDomElement start, bool goNext ) const;

private:
    mutable QString m_address;
    // Note: you can't add other member variables here, except for caching info.
    // The KBookmarks are created on the fly, as wrappers
    // around internal QDomElements. Any additional information
    // has to be implemented as an attribute of the QDomElement.
};

#endif
