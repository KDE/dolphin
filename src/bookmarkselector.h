/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef BOOKMARKSELECTOR_H
#define BOOKMARKSELECTOR_H

#include <kbookmark.h>
#include <urlbutton.h>
//Added by qt3to4:
#include <Q3PopupMenu>

class URLNavigator;
class Q3PopupMenu;
class KUrl;

/**
 * @brief Allows to select a bookmark from a popup menu.
 *
 * The icon from the current selected bookmark is shown
 * inside the bookmark selector.
 *
 * @see URLNavigator
 * @author Peter Penz <peter.penz@gmx.at>
 */
class BookmarkSelector : public URLButton
{
    Q_OBJECT

public:
    /**
     * @param parent Parent widget where the bookmark selector
     *               is embedded into.
     */
    BookmarkSelector(URLNavigator* parent);

    virtual ~BookmarkSelector();

    /**
     * Updates the selection dependent from the given URL \a url. The
     * URL must not match exactly to one of the available bookmarks:
     * The bookmark which is equal to the URL or at least is a parent URL
     * is selected. If there are more than one possible parent URL candidates,
     * the bookmark which covers the bigger range of the URL is selected.
     */
    void updateSelection(const KUrl& url);

    /**
     * Returns the index of the selected bookmark. To get
     * the bookmark, use BookmarkSelector::selectedBookmark().
     */
    int selectedIndex() const { return m_selectedIndex; }

    /** Returns the selected bookmark. */
    KBookmark selectedBookmark() const;

signals:
    /**
     * Is send when a bookmark has been activated by the user.
     * @param index Index of the selected bookmark.
     */
    void bookmarkActivated(int index);

protected:
    /**
     * Draws the icon of the selected URL as content of the URL
     * selector.
     */
    virtual void drawButton(QPainter* painter);

private slots:
    /**
     * Updates the selected index and the icon if a bookmark
     * has been activated by the user.
     */
    void slotBookmarkActivated(int index);

private:
    int m_selectedIndex;
    Q3PopupMenu* m_bookmarksMenu;

};

#endif
