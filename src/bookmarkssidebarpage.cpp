/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "bookmarkssidebarpage.h"

#include <q3listbox.h>
#include <qlayout.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3VBoxLayout>
#include <QPaintEvent>
#include <assert.h>
#include <kmenu.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klocale.h>

#include "dolphinsettings.h"
#include "editbookmarkdialog.h"

BookmarksSidebarPage::BookmarksSidebarPage(QWidget* parent) :
    SidebarPage(parent)
{
    Q3VBoxLayout* layout = new Q3VBoxLayout(this);
    m_bookmarksList = new BookmarksListBox(this);
    m_bookmarksList->setPaletteBackgroundColor(palette().brush(QPalette::Background).color());

    layout->addWidget(m_bookmarksList);
    connect(m_bookmarksList, SIGNAL(mouseButtonClicked(int, Q3ListBoxItem*, const QPoint&)),
            this, SLOT(slotMouseButtonClicked(int, Q3ListBoxItem*)));
    connect(m_bookmarksList, SIGNAL(contextMenuRequested(Q3ListBoxItem*, const QPoint&)),
            this, SLOT(slotContextMenuRequested(Q3ListBoxItem*, const QPoint&)));

    KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
    connect(manager, SIGNAL(changed(const QString&, const QString&)),
            this, SLOT(updateBookmarks()));

    updateBookmarks();
}

BookmarksSidebarPage::~BookmarksSidebarPage()
{
}

void BookmarksSidebarPage::setUrl(const KUrl& url)
{
    if (!m_url.equals(url, KUrl::CompareWithoutTrailingSlash)) {
        m_url = url;
        adjustSelection(m_url);
    }
}

void BookmarksSidebarPage::updateBookmarks()
{
    m_bookmarksList->clear();

    KIconLoader iconLoader;

    KBookmarkGroup root = DolphinSettings::instance().bookmarkManager()->root();
    KBookmark bookmark = root.first();
    while (!bookmark.isNull()) {
        QPixmap icon(iconLoader.loadIcon(bookmark.icon(),
                                         K3Icon::NoGroup,
                                         K3Icon::SizeMedium));
        BookmarkItem* item = new BookmarkItem(icon, bookmark.text());
        m_bookmarksList->insertItem(item);

        bookmark = root.next(bookmark);
    }
}

void BookmarksSidebarPage::slotMouseButtonClicked(int button, Q3ListBoxItem* item)
{
    if ((button != Qt::LeftButton) || (item == 0)) {
        return;
    }

    const int index = m_bookmarksList->index(item);
    KBookmark bookmark = DolphinSettings::instance().bookmark(index);
    emit changeUrl(bookmark.url());
}

void BookmarksSidebarPage::slotContextMenuRequested(Q3ListBoxItem* item,
                                                    const QPoint& pos)
{
    const int insertID = 1;
    const int editID = 2;
    const int deleteID = 3;
    const int addID = 4;

    KMenu* popup = new KMenu();
    if (item == 0) {
        QAction *action = popup->addAction(KIcon("document-new"), i18n("Add Bookmark..."));
	action->setData(addID);
    }
    else {
        QAction *action = popup->addAction(KIcon("document-new"), i18n("Insert Bookmark..."));
	action->setData(insertID);
        action = popup->addAction(KIcon("edit"), i18n("Edit..."));
	action->setData(editID);
        action = popup->addAction(KIcon("edit-delete"), i18n("Delete"));
	action->setData(deleteID);

    }

    KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
    KBookmarkGroup root = manager->root();
    const int index = m_bookmarksList->index(m_bookmarksList->selectedItem());
    QAction *result = popup->exec(pos);
    if( result)
    {
    switch(result->data().toInt()) {
        case insertID: {
            KBookmark newBookmark = EditBookmarkDialog::getBookmark(i18n("Insert Bookmark"),
                                                                    "New bookmark",
                                                                    KUrl(),
                                                                    "bookmark");
            if (!newBookmark.isNull()) {
                root.addBookmark(manager, newBookmark);
                if (index > 0) {
                    KBookmark prevBookmark = DolphinSettings::instance().bookmark(index - 1);
                    root.moveItem(newBookmark, prevBookmark);
                }
                else {
                    // insert bookmark at first position (is a little bit tricky as KBookmarkGroup
                    // only allows to move items after existing items)
                    KBookmark firstBookmark = root.first();
                    root.moveItem(newBookmark, firstBookmark);
                    root.moveItem(firstBookmark, newBookmark);
                }
                manager->emitChanged(root);
            }
            break;
        }

        case editID: {
            KBookmark oldBookmark = DolphinSettings::instance().bookmark(index);
            KBookmark newBookmark = EditBookmarkDialog::getBookmark(i18n("Edit Bookmark"),
                                                                    oldBookmark.text(),
                                                                    oldBookmark.url(),
                                                                    oldBookmark.icon());
            if (!newBookmark.isNull()) {
                root.addBookmark(manager, newBookmark);
                root.moveItem(newBookmark, oldBookmark);
                root.deleteBookmark(oldBookmark);
                manager->emitChanged(root);
            }
            break;
        }

        case deleteID: {
            KBookmark bookmark = DolphinSettings::instance().bookmark(index);
            root.deleteBookmark(bookmark);
            manager->emitChanged(root);
            break;
        }

        case addID: {
            KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add Bookmark"),
                                                                 "New bookmark",
                                                                 KUrl(),
                                                                 "bookmark");
            if (!bookmark.isNull()) {
                root.addBookmark(manager, bookmark);
                manager->emitChanged(root);
            }
        }

        default: break;
    }
   }
    delete popup;
    popup = 0;
}


void BookmarksSidebarPage::adjustSelection(const KUrl& url)
{
    KBookmarkGroup root = DolphinSettings::instance().bookmarkManager()->root();
    KBookmark bookmark = root.closestBookmark(url);

    const bool block = m_bookmarksList->signalsBlocked();
    m_bookmarksList->blockSignals(true);
    if (bookmark.isNull()) {
        // no bookmark matches, hence deactivate any selection
        const int currentIndex = m_bookmarksList->index(m_bookmarksList->selectedItem());
        m_bookmarksList->setSelected(currentIndex, false);
    }
    else {
        // select the bookmark which is part of the current Url
        // TODO when porting to QListWidget, use the address as item data?
        int selectedIndex = bookmark.address().mid(1).toInt(); // convert "/5" to 5.
        m_bookmarksList->setSelected(selectedIndex, true);
    }
    m_bookmarksList->blockSignals(block);
}

BookmarksListBox::BookmarksListBox(QWidget* parent) :
    Q3ListBox(parent)
{
}
BookmarksListBox::~BookmarksListBox()
{
}

void BookmarksListBox::paintEvent(QPaintEvent* /* event */)
{
    // don't invoke QListBox::paintEvent(event) to prevent
    // that any kind of frame is drawn
}

BookmarkItem::BookmarkItem(const QPixmap& pixmap, const QString& text) :
    Q3ListBoxPixmap(pixmap, text)
{
}

BookmarkItem::~BookmarkItem()
{
}

int BookmarkItem::height(const Q3ListBox* listBox) const
{
    return Q3ListBoxPixmap::height(listBox) + 8;
}

#include "bookmarkssidebarpage.moc"
