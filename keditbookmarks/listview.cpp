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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>

#include "toplevel.h"
#include "commands.h"
#include "favicons.h"
#include "listview.h"
#include "testlink.h"
#include "mymanager.h"

#define IS_EF(it) (static_cast<KEBListViewItem *>(it)->isEmptyFolder())

ListView* ListView::s_self = 0;

void ListView::initListView() {
   listView()->setRootIsDecorated(false);
   listView()->addColumn(i18n("Bookmark"), 300);
   listView()->addColumn(i18n("URL"), 300);
   listView()->addColumn(i18n("Status/Last Modified"), 300);
#ifdef DEBUG_ADDRESSES
   listView()->addColumn(i18n("Address"), 100);
#endif
   listView()->setRenameable(COL_NAME);
   listView()->setRenameable(COL_URL);
   listView()->setSorting(-1, false);
   listView()->setDragEnabled(true);
   listView()->setSelectionModeExt(KListView::Extended);
   listView()->setAllColumnsShowFocus(true);
}

void ListView::updateListViewSetup(bool readonly) {
   listView()->setItemsMovable(readonly); // we move items ourselves (for undo)
   listView()->setItemsRenameable(!readonly);
   listView()->setAcceptDrops(!readonly);
   listView()->setDropVisualizer(!readonly);
}

void ListView::connectSignals() {
   connect(listView(), SIGNAL( selectionChanged() ),
           this,       SLOT( slotSelectionChanged() ));
   connect(listView(), SIGNAL( contextMenu(KListView *, QListViewItem*, const QPoint &) ),
           this,       SLOT( slotContextMenu(KListView *, QListViewItem *, const QPoint &) ));
   connect(listView(), SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
           this,       SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ));
   connect(listView(), SIGNAL( doubleClicked(QListViewItem *, const QPoint &, int) ),
           this,       SLOT( slotDoubleClicked(QListViewItem *, const QPoint &, int) ));
   connect(listView(), SIGNAL( dropped(QDropEvent*, QListViewItem*, QListViewItem*) ),
           this,       SLOT( slotDropped(QDropEvent*, QListViewItem*, QListViewItem*) ));
}

QListViewItem* ListView::getFirstChild() {
   return listView()->firstChild();
}

QPtrList<QListViewItem>* ListView::itemList() {
   QPtrList<QListViewItem> *items = new QPtrList<QListViewItem>();
   for (QListViewItemIterator it(listView()); it.current(); it++) {
      items->append(it.current());
   }
   return items;
}

const KBookmark ListView::itemToBookmark(QListViewItem *item) {
   return static_cast<KEBListViewItem *>(item)->bookmark();
}

QValueList<KBookmark> ListView::itemsToBookmarks(QPtrList<QListViewItem>* items) {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<QListViewItem> it(*items); it.current() != 0; ++it) {
      KEBListViewItem *item = static_cast<KEBListViewItem *>(it.current());
      bookmarks.append(KBookmark(item->bookmark()));
   }
   return bookmarks;
}

QPtrList<QListViewItem>* ListView::selectedItems() {
   QPtrList<QListViewItem> *items = new QPtrList<QListViewItem>();
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->isSelected()) {
         items->append(it.current());
      }
   }
   return items;
}

QListViewItem* ListView::firstSelected() {
   return selectedItems()->first();
}

KEBListViewItem* ListView::findOpenParent(KEBListViewItem *item) {
   KEBListViewItem *c = item;
   while(true) {
      if (c = static_cast<KEBListViewItem *>(c->parent()), !c) {
         return 0;
      } else if (c->isOpen()) {
         return c;
      }
   }
}

void ListView::openParents(KEBListViewItem *item) {
   KEBListViewItem *c = item;
   while(true) {
      if (c = static_cast<KEBListViewItem *>(c->parent()), c) {
         c->setOpen(true);
      } else {
         break;
      }
   }
}

void ListView::deselectParents(QListViewItem *item) {
   QListViewItem *c = item;
   while(true) {
      if (c = c->parent(), c) {
         c->setSelected(false);
      } else {
         break;
      }
   }
}

void ListView::updateSelectedItems() {
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->isSelected()) {
         deselectParents(it.current());
      }
   }
}

KBookmark ListView::selectedBookmark() {
   return itemToBookmark(firstSelected());
}

QValueList<KBookmark> ListView::selectedBookmarksExpanded() {
   QValueList<KBookmark> bookmarks;
   QStringList addresses;
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if ((it.current()->isSelected()) && !IS_EF(it.current())) {
         if (it.current()->childCount() > 0) {
            for(QListViewItemIterator it2(it.current()); it2.current(); it2++) {
               if (!IS_EF(it2.current())) {
                  const KBookmark bk = itemToBookmark(it2.current());
                  if (!addresses.contains(bk.address())) {
                     bookmarks.append(bk);
                     addresses.append(bk.address());
                  }
               }
               if ((it.current()->nextSibling())
                && (it2.current() == it.current()->nextSibling()->itemAbove())
               ) {
                  break;
               }
            }
         } else {
            const KBookmark bk = itemToBookmark(it.current());
            if (!addresses.contains(bk.address())) {
               bookmarks.append(bk);
               addresses.append(bk.address());
            }
         }
      }
   }
   return bookmarks;
}

QValueList<KBookmark> ListView::allBookmarks() {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if ((it.current()->childCount() == 0) && !IS_EF(it.current())) {
         bookmarks.append(itemToBookmark(it.current()));
      }
   }
   return bookmarks;
}

// DESIGN - make + "/0" a kbookmark:: thing?

QString ListView::userAddress() {
   if(selectedItems()->count() == 0) {
      // FIXME - maybe a in view one?
      //       - else we could get /0
      //       - in view?
      return "/0";

   } else {
      KBookmark current = itemToBookmark(selectedItems()->first());
      return (current.isGroup()) 
           ? (current.address() + "/0")
           : KBookmark::nextAddress(current.address());
   }
}

void ListView::setCurrent(KEBListViewItem *item) {
   listView()->setCurrentItem(item);
   listView()->ensureItemVisible(item);
}

KEBListViewItem* ListView::getItemAtAddress(const QString &address) {
   QListViewItem *item = getFirstChild();

   QStringList addresses = QStringList::split('/',address); // e.g /5/10/2

   for (QStringList::Iterator it = addresses.begin(); it != addresses.end(); ++it) {
      if (item = item->firstChild(), !item) {
         return 0;
      }
      for (unsigned int i = 0; i < (*it).toUInt(); ++i) {
         if (item = item->nextSibling(), !item) {
            return 0;
         }
      }
   }
   return static_cast<KEBListViewItem *>(item);
}

KEBListViewItem* ListView::getItemRoughlyAtAddress(const QString &address) {
   return getItemAtAddress(BkManagerAccessor::mgr()->findByAddress(address, true).address());
}

void ListView::setOpen(bool open) {
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->parent()) {
         (static_cast<KEBListViewItem*>(it.current()))->setOpen(open);
      }
   }
}

void ListView::updateLastAddress() {
   QListViewItem *lastItem = 0;
   for (QPtrListIterator<QListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if ((it.current()->isSelected()) && !IS_EF(it.current())) {
         lastItem = it.current();
      }
   }
   if (lastItem) {
      m_last_selection_address = itemToBookmark(lastItem).address();
   }
}

SelcAbilities ListView::getSelectionAbilities() {
   QListViewItem *item = selectedItems()->first();

   static SelcAbilities sa = { false, false, false, false, false, false, false, false };

   if (item) {
      KBookmark nbk = itemToBookmark(item);
      sa.itemSelected   = true;
      sa.group          = nbk.isGroup();
      sa.separator      = nbk.isSeparator();
      sa.urlIsEmpty     = nbk.url().isEmpty();
      sa.singleSelect   = (!sa.multiSelect && sa.itemSelected); // oops, TODO, FIXME!
      sa.root           = (getFirstChild() == item);
      sa.multiSelect    = (selectedItems()->count() > 1);
   } else {
      kdDebug() << "no item" << endl;
   }

   sa.notEmpty = (getFirstChild()->childCount() > 0);

   return sa;
}

void ListView::slotDropped(QDropEvent *e, QListViewItem *newParent, QListViewItem *itemAfterQLVI) {
   if (!newParent) {
      // drop before root item
      return;
   }

   KEBListViewItem *itemAfter = static_cast<KEBListViewItem *>(itemAfterQLVI);

   QString newAddress 
      = (!itemAfter || itemAfter->isEmptyFolder())
      ? (static_cast<KEBListViewItem *>(newParent)->bookmark().address() + "/0")
      : (KBookmark::nextAddress(itemAfter->bookmark().address()));

   KMacroCommand *mcmd = 0;
   if (e->source() != listView()->viewport()) {
      mcmd = CmdGen::self()->insertMimeSource(i18n("Drop items"), e, newAddress);

   } else {
      QPtrList<QListViewItem> *selection = listview->selectedItems();
      QListViewItem *firstItem = selection->first();
      if (!firstItem || firstItem == itemAfterQLVI) {
         return;
      }
      bool copy = (e->action() == QDropEvent::Copy);
      mcmd = CmdGen::self()->itemsMoved(selection, newAddress, copy);
   }

   KEBTopLevel::self()->didCommand(mcmd);
}

void ListView::updateListView() {
   QPoint pos(listView()->contentsX(), listView()->contentsY());

   // get address list for selected items, make a function?
   QStringList addressList;
   QPtrList<QListViewItem> *selcItems = selectedItems();
   if (selcItems->count() != 0) {
      for (QPtrListIterator<QListViewItem> it(*selcItems); it.current() != 0; ++it) {
         QString address = itemToBookmark(static_cast<KEBListViewItem*>(it.current())).address();
         if (address != "ERROR") {
            addressList << address;
         }
      }
   }

   fillWithGroup(BkManagerAccessor::mgr()->root());

   // re-select previously selected items
   KEBListViewItem *item = 0;
   for (QStringList::Iterator ait = addressList.begin(); ait != addressList.end(); ++ait) {
      if (item = getItemAtAddress(*ait), item) {
         listView()->setSelected(item, true);
      }
   }

   // fallback, if no selected items
   if (!item) {
      item = getItemRoughlyAtAddress(m_last_selection_address);
      listView()->setSelected(item, true);
   }

   setCurrent(item);
}

void ListView::fillWithGroup(KBookmarkGroup group, KEBListViewItem *parentItem) {
   KEBListViewItem *lastItem = 0;
   if (!parentItem) {
      listView()->clear();
      KEBListViewItem *tree = new KEBListViewItem(listView(), group);
      fillWithGroup(group, tree);
      tree->QListViewItem::setOpen(true);
      return;
   }
   for (KBookmark bk = group.first(); !bk.isNull(); bk = group.next(bk)) {
      KEBListViewItem *item = 0;
      if (bk.isGroup()) {
         KBookmarkGroup grp = bk.toGroup();
         item = new KEBListViewItem(parentItem, lastItem, grp);
         lastItem = item;
         fillWithGroup(grp, item);
         if (grp.isOpen()) {
            item->QListViewItem::setOpen(true);
         }
         if (grp.first().isNull()) {
            // empty folder
            new KEBListViewItem(item, item); 
         }

      } else {
         item = new KEBListViewItem(parentItem, lastItem, bk);
         lastItem = item;
      }
   }
}

void ListView::slotSelectionChanged() {
   KEBTopLevel::self()->updateActions();
   updateSelectedItems();
}

void ListView::slotContextMenu(KListView *, QListViewItem *item, const QPoint &p) {
   if (!item) {
      return;
   }
   const char *type = (itemToBookmark(item).isGroup() ? "popup_folder" : "popup_bookmark");
   QWidget* popup = KEBTopLevel::self()->popupMenuFactory(type);
   if (popup) {
      static_cast<QPopupMenu*>(popup)->popup(p);
   }
}

void ListView::slotDoubleClicked(QListViewItem *item, const QPoint &, int column) {
   if ((!KEBTopLevel::self()->readonly()) 
    && (item)
    && ((column == COL_URL) 
     || (column == COL_NAME))
   ) {
      listView()->rename(item, column);
   }
}

void ListView::slotItemRenamed(QListViewItem *item, const QString &newText, int column) {
   Q_ASSERT(item);
   KBookmark bk = itemToBookmark(item);
   KCommand *cmd = 0;
   switch (column) {
      case COL_NAME:
         if (newText.isEmpty()) {
            // can't have an empty name, therefore undo the user action
            item->setText(COL_NAME, bk.fullText());
         } else if (bk.fullText() != newText) {
            cmd = new RenameCommand(bk.address(), newText);
         }
         break;

      case COL_URL:
         if (bk.url() != newText) {
            cmd = new EditCommand(bk.address(), EditCommand::Edition("href", newText), i18n("URL"));
         }
         break;

      default:
         kdWarning() << "No such column " << column << endl;
         return;
   }
   KEBTopLevel::self()->addCommand(cmd);
}

void KEBListView::rename(QListViewItem *qitem, int column) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   if ( (item != firstChild()) 
     && !item->bookmark().isSeparator()
     && !((column == 1) && item->bookmark().isGroup())
   ) {
      KListView::rename(item, column);
   }
}

bool KEBListView::acceptDrag(QDropEvent * e) const {
   return (e->source() == viewport() || KBookmarkDrag::canDecode(e));
}

QDragObject *KEBListView::dragObject() {
   QPtrList<QListViewItem> *selcItems = listview->selectedItems();
   if (selcItems->count() == 0) {
      return (QDragObject*)0;
   } else {
      QValueList<KBookmark> bookmarks = listview->itemsToBookmarks(selcItems);
      KBookmarkDrag *drag = KBookmarkDrag::newDrag(bookmarks, viewport());
      const QString iconname = (bookmarks.size() == 1) ? bookmarks.first().icon() : "bookmark";
      drag->setPixmap(SmallIcon(iconname)) ;
      return drag;
   }
}

void KEBListViewItem::setOpen(bool open) {
   m_bookmark.internalElement().setAttribute("folded", open ? "no" : "yes");
   QListViewItem::setOpen(open);
}

void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &ocg, int col, int w, int a) {
   QColorGroup cg(ocg);

   if (col == COL_STAT) {
      TestLinkItr::paintCellHelper(p, cg, m_paintstyle);
   }

   QListViewItem::paintCell(p, cg, col, w,a);
}

#include "listview.moc"
