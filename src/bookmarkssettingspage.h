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

#ifndef BOOKMARKSSETTINGSPAGE_H
#define BOOKMARKSSETTINGSPAGE_H

#include <settingspagebase.h>
#include <q3valuelist.h>

class KListView;
class KPushButton;
class Q3ListViewItem;

/**
 * @brief Represents the page from the Dolphin Settings which allows
 *        to modify the bookmarks.
 */
class BookmarksSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    BookmarksSettingsPage(QWidget* parent);

    virtual ~BookmarksSettingsPage();

    /** @see SettingsPageBase::applySettings */
    virtual void applySettings();

private slots:
    void updateButtons();
    void slotBookmarkDoubleClicked(Q3ListViewItem*, const QPoint&, int);
    void slotAddButtonClicked();
    void slotEditButtonClicked();
    void slotRemoveButtonClicked();
    void slotMoveUpButtonClicked();
    void slotMoveDownButtonClicked();

    /**
     * Is connected with the signal QListView::pressed(QListViewItem* item)
     * and assures that always one bookmarks stays selected although a
     * click has been done on the viewport area.
     * TODO: this is a workaround, possibly there is a more easy approach
     * doing this...
     */
    void slotBookmarkPressed(Q3ListViewItem* item);

private:
    enum ColumnIndex {
        PixmapIdx = 0,
        NameIdx   = 1,
        URLIdx    = 2,
        IconIdx   = 3
    };

    KListView* m_listView;
    KPushButton* m_addButton;
    KPushButton* m_editButton;
    KPushButton* m_removeButton;
    KPushButton* m_moveUpButton;
    KPushButton* m_moveDownButton;

    /**
     * Returns the index of the selected bookmark
     * inside the bookmarks listview.
     */
    int selectedBookmarkIndex() const;

    /**
     * Moves the currently selected bookmark up, if 'direction'
     * is < 0, otherwise the bookmark is moved down.
     */
    void moveBookmark(int direction);
};

#endif
