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
#include <klineedit.h>
#include <krun.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

#include "toplevel.h"
#include "commands.h"
#include "mymanager.h"
#include "testlink.h" // paintCellHelper

#include "listview.h"

// #define DEBUG_ADDRESSES

ListView* ListView::s_self = 0;

ListView::ListView() {
   ;
}

void ListView::createListView(QWidget *parent) {
   s_self = new ListView();
   self()->m_listView = new KEBListView(parent);
}

void ListView::initListView() {
   m_listView->setRootIsDecorated(false);
   m_listView->addColumn(i18n("Bookmark"), 300);
   m_listView->addColumn(i18n("URL"), 300);
   m_listView->addColumn(i18n("Comment"), 300);
   m_listView->addColumn(i18n("Status/Last Modified"), 300);
#ifdef DEBUG_ADDRESSES
   m_listView->addColumn(i18n("Address"), 100);
#endif
   m_listView->setRenameable(KEBListView::NameColumn);
   m_listView->setRenameable(KEBListView::UrlColumn);
   m_listView->setRenameable(KEBListView::CommentColumn);
   m_listView->setTabOrderedRenaming(false);
   m_listView->setSorting(-1, false);
   m_listView->setDragEnabled(true);
   m_listView->setSelectionModeExt(KListView::Extended);
   m_listView->setAllColumnsShowFocus(true);
}

void ListView::updateListViewSetup(bool readonly) {
   m_listView->setItemsMovable(readonly); // we move items ourselves (for undo)
   m_listView->setItemsRenameable(!readonly);
   m_listView->setAcceptDrops(!readonly);
   m_listView->setDropVisualizer(!readonly);
}

void ListView::connectSignals() {
   connect(m_listView, SIGNAL( selectionChanged() ),
           this,       SLOT( slotSelectionChanged() ));
   connect(m_listView, SIGNAL( contextMenu(KListView *, QListViewItem*, const QPoint &) ),
           this,       SLOT( slotContextMenu(KListView *, QListViewItem *, const QPoint &) ));
   connect(m_listView, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
           this,       SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ));
   connect(m_listView, SIGNAL( doubleClicked(QListViewItem *, const QPoint &, int) ),
           this,       SLOT( slotDoubleClicked(QListViewItem *, const QPoint &, int) ));
   connect(m_listView, SIGNAL( dropped(QDropEvent*, QListViewItem*, QListViewItem*) ),
           this,       SLOT( slotDropped(QDropEvent*, QListViewItem*, QListViewItem*) ));
}

KEBListViewItem* ListView::getFirstChild() {
   return static_cast<KEBListViewItem *>(m_listView->firstChild());
}

QPtrList<KEBListViewItem>* ListView::itemList() {
   QPtrList<KEBListViewItem> *items = new QPtrList<KEBListViewItem>();
   for (QListViewItemIterator it(m_listView); it.current(); it++) {
      items->append(static_cast<KEBListViewItem *>(it.current()));
   }
   return items;
}

QValueList<KBookmark> ListView::itemsToBookmarks(QPtrList<KEBListViewItem>* items) {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*items); it.current() != 0; ++it) {
      bookmarks.append(KBookmark(it.current()->bookmark()));
   }
   return bookmarks;
}

QPtrList<KEBListViewItem>* ListView::selectedItems() {
   QPtrList<KEBListViewItem> *items = new QPtrList<KEBListViewItem>();
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->isSelected()) {
         items->append(it.current());
      }
   }
   return items;
}

KEBListViewItem* ListView::firstSelected() {
   return selectedItems()->first();
}

KEBListViewItem* ListView::findOpenParent(KEBListViewItem *item) {
   QListViewItem *c = item;
   while(true) {
      if (c = c->parent(), !c) {
         return 0;
      } else if (c->isOpen()) {
         return static_cast<KEBListViewItem*>(c);
      }
   }
}

void ListView::openParents(KEBListViewItem *item) {
   QListViewItem *c = item;
   while(true) {
      if (c = c->parent(), c) {
         c->setOpen(true);
      } else {
         break;
      }
   }
}

void ListView::deselectParents(KEBListViewItem *item) {
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
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->isSelected()) {
         deselectParents(it.current());
      }
   }
}

QValueList<KBookmark> ListView::selectedBookmarksExpanded() {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (!it.current()->isSelected() 
       || it.current()->isEmptyFolder()
       || it.current() == getFirstChild()) {
         continue;
      }
      if (it.current()->childCount() > 0) {
         QListViewItem *endOfFolder = 0;
         if (it.current()->nextSibling()) {
            endOfFolder = it.current()->nextSibling()->itemAbove();
         }
         // TODO check that this loop covers all, included subfolders!
         for(QListViewItemIterator it2((QListViewItem*)it.current()); it2.current(); it2++) {
            KEBListViewItem *item = static_cast<KEBListViewItem *>(it2.current());
            if (!item->isEmptyFolder()) {
               bookmarks.append(item->bookmark());
            }
            if (endOfFolder && it2.current() == endOfFolder) {
               break;
            }
         }
      } else {
         bookmarks.append(it.current()->bookmark());
      }
   }
   return bookmarks;
}

QValueList<KBookmark> ListView::allBookmarks() {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if ((it.current()->childCount() == 0) && !it.current()->isEmptyFolder()) {
         bookmarks.append(it.current()->bookmark());
      }
   }
   return bookmarks;
}

void ListView::updateLastAddress() {
   KEBListViewItem *lastItem = 0;
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if ((it.current()->isSelected()) && !it.current()->isEmptyFolder()) {
         lastItem = it.current();
      }
   }
   if (lastItem) {
      m_last_selection_address = lastItem->bookmark().address();
   }
}

// DESIGN - make + "/0" a kbookmark:: thing?

QString ListView::userAddress() {
   if(selectedItems()->count() == 0) {
      // FIXME - maybe a in view one?
      //       - else we could get /0
      //       - in view?
      return "/0";
   }

   KEBListViewItem *item = firstSelected();
   if (item->isEmptyFolder()) {
      item = item->parent();
   }

   KBookmark current = item->bookmark();
   if (!current.hasParent()) {
      // oops!
      return "/0";
   }
   
   return (current.isGroup()) 
        ? (current.address() + "/0")
        : KBookmark::nextAddress(current.address());
}

void ListView::setCurrent(KEBListViewItem *item) {
   m_listView->setCurrentItem(item);
   m_listView->ensureItemVisible(item);
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

void ListView::setOpen(bool open) {
   for (QPtrListIterator<KEBListViewItem> it(*itemList()); it.current() != 0; ++it) {
      if (it.current()->parent()) {
         it.current()->setOpen(open);
      }
   }
}

SelcAbilities ListView::getSelectionAbilities() {
   KEBListViewItem *item = firstSelected();

   static SelcAbilities sa = { false, false, false, false, false, false, false, false };

   if (item) {
      KBookmark nbk = item->bookmark();
      sa.itemSelected   = true;
      sa.group          = nbk.isGroup();
      sa.separator      = nbk.isSeparator();
      sa.urlIsEmpty     = nbk.url().isEmpty();
      sa.singleSelect   = (!sa.multiSelect && sa.itemSelected); // oops, TODO, FIXME!
      sa.root           = (getFirstChild() == item);
      sa.multiSelect    = (selectedItems()->count() > 1);
   } else {
      // kdDebug() << "no item" << endl;
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
   if (e->source() != m_listView->viewport()) {
      mcmd = CmdGen::self()->insertMimeSource(i18n("Drop items"), e, newAddress);

   } else {
      QPtrList<KEBListViewItem> *selection = selectedItems();
      KEBListViewItem *firstItem = selection->first();
      if (!firstItem || firstItem == itemAfterQLVI) {
         return;
      }
      // TODO - fix the stupid bug
      bool copy = (e->action() == QDropEvent::Copy);
      mcmd = CmdGen::self()->itemsMoved(selection, newAddress, copy);
   }

   KEBApp::self()->didCommand(mcmd);
}


void ListView::updateListView() {
   // get address list for selected items, make a function?
   QStringList addressList;
   QPtrList<KEBListViewItem> *selcItems = selectedItems();
   if (selcItems->count() != 0) {
      for (QPtrListIterator<KEBListViewItem> it(*selcItems); it.current() != 0; ++it) {
         if (it.current()->bookmark().hasParent()) {
            addressList << it.current()->bookmark().address();
         }
      }
   }

   fillWithGroup(CurrentMgr::self()->mgr()->root());

   // re-select previously selected items
   KEBListViewItem *item = 0;
   for (QStringList::Iterator ait = addressList.begin(); ait != addressList.end(); ++ait) {
      if (item = getItemAtAddress(*ait), item) {
         m_listView->setSelected(item, true);
      }
   }

   // fallback, if no selected items
   if (!item) {
      QString addr = 
         CurrentMgr::self()->correctAddress(m_last_selection_address);
      item = getItemAtAddress(addr);
      m_listView->setSelected(item, true);
   }

   setCurrent(item);
}

void ListView::fillWithGroup(KBookmarkGroup group, KEBListViewItem *parentItem) {
   if (!parentItem) {
      m_listView->clear();
      KEBListViewItem *tree = new KEBListViewItem(m_listView, group);
      fillWithGroup(group, tree);
      tree->QListViewItem::setOpen(true);
      return;
   }
   KEBListViewItem *lastItem = 0;
   for (KBookmark bk = group.first(); !bk.isNull(); bk = group.next(bk)) {
      KEBListViewItem *item = 0;
      if (bk.isGroup()) {
         KBookmarkGroup grp = bk.toGroup();
         item = new KEBListViewItem(parentItem, lastItem, grp);
         fillWithGroup(grp, item);
         if (grp.isOpen()) {
            item->QListViewItem::setOpen(true);
         }
         if (grp.first().isNull()) {
            // empty folder
            new KEBListViewItem(item, item); 
         }
         lastItem = item;

      } else {
         item = lastItem ? new KEBListViewItem(parentItem, lastItem, bk)
                         : new KEBListViewItem(parentItem, bk);
         lastItem = item;
      }
   }
}

void ListView::slotSelectionChanged() {
   KEBApp::self()->updateActions();
   updateSelectedItems();
}

void ListView::slotContextMenu(KListView *, QListViewItem *qitem, const QPoint &p) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   if (!item) {
      return;
   }
   const char *type = 
      (item == getFirstChild()) || (item->bookmark().isGroup()) 
    ? "popup_folder" : "popup_bookmark";
   QWidget* popup = KEBApp::self()->popupMenuFactory(type);
   if (popup) {
      static_cast<QPopupMenu*>(popup)->popup(p);
   }
}

void ListView::slotDoubleClicked(QListViewItem *item, const QPoint &, int column) {
   m_listView->rename(item, column);
}

void ListView::slotItemRenamed(QListViewItem *item, const QString &newText, int column) {
   Q_ASSERT(item);
   KBookmark bk = static_cast<KEBListViewItem *>(item)->bookmark();
   KCommand *cmd = 0;
   switch (column) {
      case KEBListView::NameColumn:
         if (newText.isEmpty()) {
            // can't have an empty name, therefore undo the user action
            item->setText(KEBListView::NameColumn, bk.fullText());
         } else if (bk.fullText() != newText) {
            cmd = new RenameCommand(bk.address(), newText);
         }
         break;

      case KEBListView::UrlColumn:
         if (bk.url() != newText) {
            cmd = new EditCommand(bk.address(), EditCommand::Edition("href", newText), i18n("URL"));
         }
         break;

      case KEBListView::CommentColumn:
         if (bk.internalElement().attribute("desc") != newText) {
            cmd = new EditCommand(bk.address(), EditCommand::Edition("desc", newText), i18n("Comment"));
         }
         break;

      default:
         kdWarning() << "No such column " << column << endl;
         return;
   }
   KEBApp::self()->addCommand(cmd);
}

// used by f2 and f3 shortcut slots - see actionsimpl
void ListView::rename(int column) {
   m_listView->rename(firstSelected(), column);
}

void ListView::clearSelection() {
   m_listView->clearSelection();
}

/* -------------------------------------- */

class KeyPressEater : public QObject {
public:
   KeyPressEater( QWidget *parent = 0, const char *name = 0 ) { ; }
protected:
   bool eventFilter(QObject *, QEvent *);
};

static int myrenamecolumn = -1;
static KEBListViewItem *myrenameitem = 0;

void ListView::renameNextCell(bool fwd) {
   while (1) {
      if (fwd && myrenamecolumn < KEBListView::CommentColumn) {
         myrenamecolumn++;
      } else if (!fwd && myrenamecolumn > KEBListView::NameColumn) {
         myrenamecolumn--;
      } else {
         myrenameitem    = static_cast<KEBListViewItem *>(
                           fwd ? myrenameitem->itemBelow()
                               : myrenameitem->itemAbove());
         if (!myrenameitem) {
            myrenameitem = fwd ? m_listView->firstChild()
                               : m_listView->lastItem();
         }
         myrenamecolumn  = fwd ? KEBListView::NameColumn 
                               : KEBListView::CommentColumn;
      }
      if (!myrenameitem 
        || myrenameitem == getFirstChild() 
        || myrenameitem->isEmptyFolder()
        || myrenameitem->bookmark().isSeparator()
        || (myrenamecolumn == KEBListView::UrlColumn 
         && myrenameitem->bookmark().isGroup())
      ) {
         ;
      } else {
         break;
      }
   }
   m_listView->rename(myrenameitem, myrenamecolumn);
}

void KEBListView::rename(QListViewItem *qitem, int column) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   if ( !(column == NameColumn || column == UrlColumn || column == CommentColumn)
     || KEBApp::self()->readonly()
     || !item 
     || item == firstChild() 
     || item->isEmptyFolder()
     || item->bookmark().isSeparator()
     || (column == UrlColumn && item->bookmark().isGroup())
   ) {
      return;
   }
   myrenamecolumn = column;
   myrenameitem = item;
   KeyPressEater *keyPressEater = new KeyPressEater(this);
   renameLineEdit()->installEventFilter(keyPressEater);
   KListView::rename(item, column);
}

bool KeyPressEater::eventFilter(QObject *, QEvent *pe) {
   if (pe->type() == QEvent::KeyPress) {
      QKeyEvent *k = (QKeyEvent *) pe;
      if ((k->key() == Qt::Key_Backtab || k->key() == Qt::Key_Tab)
      && !(k->state() & ControlButton || k->state() & AltButton)
      ) {
         bool fwd = (k->key() == Key_Tab && !(k->state() & ShiftButton));
         ListView::self()->renameNextCell(fwd);
         return true;
      }
   }
   return false;
}

bool KEBListView::acceptDrag(QDropEvent * e) const {
   return (e->source() == viewport() || KBookmarkDrag::canDecode(e));
}

QDragObject *KEBListView::dragObject() {
   QPtrList<KEBListViewItem> *selcItems = ListView::self()->selectedItems();
   if (selcItems->count() == 0) {
      return (QDragObject*)0;
   }
   QValueList<KBookmark> bookmarks = ListView::self()->itemsToBookmarks(selcItems);
   KBookmarkDrag *drag = KBookmarkDrag::newDrag(bookmarks, viewport());
   const QString iconname = (bookmarks.size() == 1) ? bookmarks.first().icon() : "bookmark";
   drag->setPixmap(SmallIcon(iconname)) ;
   return drag;
}

/* -------------------------------------- */

void KEBListViewItem::normalConstruct(const KBookmark &bk) {
#ifdef DEBUG_ADDRESSES
   setText(KEBListView::AddressColumn, bk.address());
#endif
   setText(KEBListView::CommentColumn, bk.internalElement().attribute("desc"));
   setPixmap(0, SmallIcon(bk.icon()));
   modUpdate();
}

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark &group)
   : QListViewItem(parent, i18n("Bookmarks")), m_bookmark(group), m_emptyFolder(false) {

   setPixmap(0, SmallIcon("bookmark"));
   setExpandable(true);
}

// empty folder item
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after)
    : QListViewItem(parent, after, i18n("Empty folder") ), m_emptyFolder(true) {

   setPixmap(0, SmallIcon("bookmark"));
}

// group
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup &gp)
   : QListViewItem(parent, after, gp.fullText()), m_bookmark(gp), m_emptyFolder(false) {

   setExpandable(true);
   normalConstruct(gp);
}

// bookmark (first of its group)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk)
   : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolder(false) {

   normalConstruct(bk);
}

// bookmark (after another)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark &bk)
   : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolder(false) {
   normalConstruct(bk);
}

void KEBListViewItem::setOpen(bool open) {
   m_bookmark.internalElement().setAttribute("folded", open ? "no" : "yes");
   QListViewItem::setOpen(open);
}

void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &ocg, int col, int w, int a) {
   QColorGroup cg(ocg);

   if (col == KEBListView::StatusColumn) {
      TestLinkItr::paintCellHelper(p, cg, m_paintstyle);
   }

   QListViewItem::paintCell(p, cg, col, w,a);
}

#include "listview.moc"
