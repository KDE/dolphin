/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef BOOKMARKSELECTOR_H
#define BOOKMARKSELECTOR_H

#include <kbookmark.h>
#include <urlbutton.h>

class UrlNavigator;
class KMenu;
class KUrl;

/**
 * @brief Allows to select a bookmark from a popup menu.
 *
 * The icon from the current selected bookmark is shown
 * inside the bookmark selector.
 *
 * @see UrlNavigator
 */
class BookmarkSelector : public UrlButton
{
    Q_OBJECT

public:
    /**
     * @param parent Parent widget where the bookmark selector
     *               is embedded into.
     */
    BookmarkSelector(UrlNavigator* parent, KBookmarkManager* bookmarkManager);

    virtual ~BookmarkSelector();

    /**
     * Updates the selection dependent from the given URL \a url. The
     * URL must not match exactly to one of the available bookmarks:
     * The bookmark which is equal to the URL or at least is a parent URL
     * is selected. If there are more than one possible parent URL candidates,
     * the bookmark which covers the bigger range of the URL is selected.
     */
    void updateSelection(const KUrl& url);

    /** Returns the selected bookmark. */
    KBookmark selectedBookmark() const;

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

signals:
    /**
     * Is send when a bookmark has been activated by the user.
     * @param url URL of the selected bookmark.
     */
    void bookmarkActivated(const KUrl& url);

protected:
    /**
     * Draws the icon of the selected Url as content of the Url
     * selector.
     */
    virtual void paintEvent(QPaintEvent* event);

private slots:
    /**
     * Updates the selected index and the icon to the bookmark
     * which is indicated by the triggered action \a action.
     */
    void activateBookmark(QAction* action);

private:
    QString m_selectedAddress;
    UrlNavigator* m_urlNavigator;
    KMenu* m_bookmarksMenu;
    KBookmarkManager* m_bookmarkManager;
};

#endif
