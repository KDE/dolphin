/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
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
#ifndef _BOOKMARKSSIDEBARPAGE_H_
#define _BOOKMARKSSIDEBARPAGE_H_

#include <sidebarpage.h>
#include <q3listbox.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QPixmap>

class KUrl;
class BookmarksListBox;

/**
 * @brief Sidebar page for accessing bookmarks.
 *
 * It is possible to add, remove and edit bookmarks
 * by a context menu. The selection of the bookmark
 * is automatically adjusted to the Url given by
 * the active view.
 */
class BookmarksSidebarPage : public SidebarPage
{
        Q_OBJECT

public:
    BookmarksSidebarPage(QWidget* parent);
    virtual ~BookmarksSidebarPage();

protected:
    /** @see SidebarPage::activeViewChanged() */
    virtual void activeViewChanged();

private slots:
    /** Fills the listbox with the bookmarks stored in DolphinSettings. */
    void updateBookmarks();

    /**
     * Checks whether the left mouse button has been clicked above a bookmark.
     * If this is the case, the Url for the currently active view is adjusted.
     */
    void slotMouseButtonClicked(int button, Q3ListBoxItem* item);

    /** @see QListBox::slotContextMenuRequested */
    void slotContextMenuRequested(Q3ListBoxItem* item, const QPoint& pos);

    /**
     * Is invoked whenever the Url of the active view has been changed. Adjusts
     * the selection of the listbox to the bookmark which is part of the current Url.
     */
    void slotUrlChanged(const KUrl& url);

private:
    /**
     * Updates the selection dependent from the given Url \a url. The
     * Url must not match exactly to one of the available bookmarks:
     * The bookmark which is equal to the Url or at least is a parent Url
     * is selected. If there are more than one possible parent Url candidates,
     * the bookmark which covers the bigger range of the Url is selected.
     */
    void adjustSelection(const KUrl& url);

    /**
     * Connects to signals from the currently active Dolphin view to get
     * informed about Url and bookmark changes.
     */
    void connectToActiveView();

    BookmarksListBox* m_bookmarksList;
};

/**
 * @brief Listbox which contains a list of bookmarks.
 *
 * Only QListBox::paintEvent() has been overwritten to prevent
 * that a (not wanted) frameborder is drawn.
 */
class BookmarksListBox : public Q3ListBox
{
    Q_OBJECT

public:
    BookmarksListBox(QWidget* parent);
    virtual ~BookmarksListBox();

protected:
    /** @see QWidget::paintEvent() */
    virtual void paintEvent(QPaintEvent* event);
};

/**
 * @brief Item which can be added to a BookmarksListBox.
 *
 * Only QListBoxPixmap::height() has been overwritten to get
 * a spacing between the items.
 */
class BookmarkItem : public Q3ListBoxPixmap
{
public:
    BookmarkItem(const QPixmap& pixmap, const QString& text);
    virtual ~BookmarkItem();
    virtual int height(const Q3ListBox* listBox) const;
};

#endif // _BOOKMARKSSIDEBARPAGE_H_
