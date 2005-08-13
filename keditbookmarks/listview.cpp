// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/*void ListView::handleDropped(KEBListView *, QDropEvent *e, Q3ListViewItem *newParent, Q3ListViewItem *itemAfterQLVI) {

    bool inApp = (e->source() == m_listView->viewport());

    // drop before root item
    if (!newParent)
        return;

    KEBListViewItem *itemAfter = static_cast<KEBListViewItem *>(itemAfterQLVI);

    QString newAddress 
        = (!itemAfter || itemAfter->isEmptyFolderPadder())
        ? (static_cast<KEBListViewItem *>(newParent)->bookmark().address() + "/0")
        : (KBookmark::nextAddress(itemAfter->bookmark().address()));

    KEBMacroCommand *mcmd = 0;

    if (!inApp) {
        mcmd = CmdGen::insertMimeSource(i18n("Drop Items"), e, newAddress);

    } else {
        if (!(mSelectedItems.count() > 0) || (firstSelected() == itemAfterQLVI))
            return;
        bool copy = (e->action() == QDropEvent::Copy);
        mcmd = CmdGen::itemsMoved(selectedItemsMap(), newAddress, copy);
    }

    CmdHistory::self()->didCommand(mcmd);

}
*/

// Update display of bookmarks containing URL
/*
void KEBListView::updateByURL(QString url) {
    for (Q3ListViewItemIterator it(this); it.current(); it++) {
        KEBListViewItem *p = static_cast<KEBListViewItem *>(it.current());
        if (p->text(1) == url) {
            p->modUpdate();
        }
    }
}
*/

/*bool KEBListView::acceptDrag(QDropEvent * e) const {
    return (e->source() == viewport() || KBookmarkDrag::canDecode(e));
}
*/

/*
Q3DragObject *KEBListView::dragObject() {
    Q3ValueList<KBookmark> bookmarks = 
        ListView::self()->itemsToBookmarks(ListView::self()->selectedItemsMap());
    KBookmarkDrag *drag = KBookmarkDrag::newDrag(bookmarks, viewport());
    const QString iconname = 
        (bookmarks.size() == 1) ? bookmarks.first().icon() : QString("bookmark");
    drag->setPixmap(SmallIcon(iconname)) ;
    return drag;
    return 0L;
}
*/
