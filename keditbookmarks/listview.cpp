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
#include "testlink.h" // paintCellHelper

#include "listview.h"

// #define DEBUG_ADDRESSES

ListView* ListView::s_self = 0;

ListView::ListView() {
   m_splitView = false;
}

void ListView::createListViews(QSplitter *splitter) {
   (void) self();
   self()->m_folderListView = self()->m_splitView ? new KEBListView(splitter, true) : 0;
   self()->m_listView = new KEBListView(splitter, false);
   splitter->setSizes(QValueList<int>() << 100 << 300);
}

void ListView::initListViews() {
   self()->m_listView->init();
   if (m_splitView) {
      self()->m_folderListView->init();
   }
}

void KEBListView::init() {
   setRootIsDecorated(false);
   if (!m_folderList) {
      addColumn(i18n("Bookmark"), 300);
      addColumn(i18n("URL"), 300);
      addColumn(i18n("Comment"), 300);
      addColumn(i18n("Status/Last Modified"), 300);
#ifdef DEBUG_ADDRESSES
      addColumn(i18n("Address"), 100);
#endif
   } else {
      addColumn(i18n("Folder"), 300);
   }
   setRenameable(KEBListView::NameColumn);
   setRenameable(KEBListView::UrlColumn);
   setRenameable(KEBListView::CommentColumn);
   setTabOrderedRenaming(false);
   setSorting(-1, false);
   setDragEnabled(true);
   setSelectionModeExt((!m_folderList) ? KListView::Extended: KListView::Single);
   setAllColumnsShowFocus(true);
}

void ListView::updateListViewSetup(bool readonly) {
   self()->m_listView->readonlyFlagInit(readonly);
   if (m_splitView) {
      self()->m_folderListView->readonlyFlagInit(readonly);
   }
}

void KEBListView::readonlyFlagInit(bool readonly) {
   setItemsMovable(readonly); // we move items ourselves (for undo)
   setItemsRenameable(!readonly);
   setAcceptDrops(!readonly);
   setDropVisualizer(!readonly);
}

void ListView::setInitialAddress(QString address) {
   m_last_selection_address = address;
}

void ListView::connectSignals() {
   connectSignals(m_listView);
   if (m_splitView) {
      connectSignals(m_folderListView);
   }
}

// forwards - fixme - this should actually be used :)

void KEBListView::slotSelectionChanged() { 
   ListView::self()->handleSelectionChanged(this); 
}

void KEBListView::slotCurrentChanged(QListViewItem *a) { 
   ListView::self()->handleCurrentChanged(this, a); 
}

void KEBListView::slotContextMenu(KListView *a, QListViewItem *b, const QPoint &c) {
   ListView::self()->handleContextMenu(this, a,b,c); 
}

void KEBListView::slotItemRenamed(QListViewItem *a, const QString &b, int c) { 
   ListView::self()->handleItemRenamed(this, a,b,c); 
}

void KEBListView::slotDoubleClicked(QListViewItem *a, const QPoint &b, int c) { 
   ListView::self()->handleDoubleClicked(this, a,b,c); 
}

void KEBListView::slotDropped(QDropEvent *a, QListViewItem *b, QListViewItem *c) { 
   ListView::self()->handleDropped(this, a,b,c); 
}

void KEBListView::startDrag() {
   QDragObject *drag = dragObject();

   if (!drag) {
      return;
   }

   /*bool moved = */ drag->drag();

   /*
   kdDebug() << "1" << endl;
   if (moved) {
      kdDebug() << moved << ", " << drag->target() << ", " << viewport() << endl;
      kdDebug() << "cooool, gonna delete it!" << endl;
      if (drag->target() != viewport()) {
         KMacroCommand *mcmd = CmdGen::self()->deleteItems( i18n("Moved Items"), 
                                                            ListView::self()->selectedItems());
         CmdHistory::self()->didCommand(mcmd);
      }
   }
   kdDebug() << "2" << endl;
   */
}

void ListView::connectSignals(KEBListView *listview) {
   connect(listview, SIGNAL( selectionChanged() ),
           listview, SLOT( slotSelectionChanged() ));
   connect(listview, SIGNAL( currentChanged(QListViewItem *) ),
           listview, SLOT( slotCurrentChanged(QListViewItem *) ));
   connect(listview, SIGNAL( contextMenu(KListView *, QListViewItem*, const QPoint &) ),
           listview, SLOT( slotContextMenu(KListView *, QListViewItem *, const QPoint &) ));
   connect(listview, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
           listview, SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ));
   connect(listview, SIGNAL( doubleClicked(QListViewItem *, const QPoint &, int) ),
           listview, SLOT( slotDoubleClicked(QListViewItem *, const QPoint &, int) ));
   connect(listview, SIGNAL( dropped(QDropEvent*, QListViewItem*, QListViewItem*) ),
           listview, SLOT( slotDropped(QDropEvent*, QListViewItem*, QListViewItem*) ));
}

QValueList<KBookmark> ListView::itemsToBookmarks(QPtrList<KEBListViewItem>* items) {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*items); it.current() != 0; ++it) {
      bookmarks.append(KBookmark(it.current()->bookmark()));
   }
   return bookmarks;
}

static QPtrList<KEBListViewItem>* selected_items_cache = 0;
static bool listview_is_dirty = false;

QPtrList<KEBListViewItem>* ListView::selectedItems() {
   if (!selected_items_cache || listview_is_dirty) {
      QPtrList<KEBListViewItem> *items = new QPtrList<KEBListViewItem>();
      for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList()));
           it.current() != 0; ++it) {
         if (it.current()->isSelected()) {
            items->append(it.current());
         }
      }
      selected_items_cache = items;
   }
   return selected_items_cache;
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

enum Which { None, Some, All };

static Which whichChildrenSelected(KEBListViewItem *item) {
   bool some = false;
   bool all = true;
   QListViewItem *endOfFolder
      = item->nextSibling() ? item->nextSibling()->itemAbove() : 0;
   QListViewItemIterator it((QListViewItem*)item);
   it++;
   QListViewItem *last = 0;
   for( ; it.current() && (last != endOfFolder); (last = it.current()), it++) {
      KEBListViewItem *item = static_cast<KEBListViewItem *>(it.current());
      if (!item->isEmptyFolder()) {
         if (item->isSelected()) {
            some = true;
         } else {
            all = false;
         }
      }
   }
   return all ? All : (some ? Some : None);
}

static void deselectAllButParent(KEBListViewItem *item) {
   QListViewItem *endOfFolder
      = item->nextSibling() ? item->nextSibling()->itemAbove() : 0;
   QListViewItemIterator it((QListViewItem*)item);
   it++;
   QListViewItem *last = 0;
   for( ; it.current() && (last != endOfFolder); (last = it.current()), it++) {
      KEBListViewItem *item = static_cast<KEBListViewItem *>(it.current());
      if (!item->isEmptyFolder() && item->isSelected()) {
         it.current()->setSelected(false);
      }
   }
   item->setSelected(true);
}

void ListView::updateSelectedItems() {
   bool selected = false;

   // adjust the current selection
   QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList()));
   for ( ; it.current() != 0; ++it) {
      if (!it.current()->isEmptyFolder() && it.current()->isSelected()) {
         selected = true;
      } else {
         continue;
      }
      if (it.current()->childCount() == 0) {
         // don't bother looking into it if its not a folder
         continue;
      }
      int which = whichChildrenSelected(it.current());
      if (which == All) { 
         // select outer folder
         deselectAllButParent(it.current());
      } else if (which == Some) { 
         // don't select outer folder
         it.current()->setSelected(false);
      }
   }

   // deselect empty folders if there is a real selection
   if (!selected) {
      return;
   }
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); 
        it.current() != 0; ++it) {
      if (it.current()->isEmptyFolder()) {
         it.current()->setSelected(false);
      }
   }
}

QValueList<KBookmark> ListView::selectedBookmarksExpanded() {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); it.current() != 0; ++it) {
      if (!it.current()->isSelected() 
       || it.current()->isEmptyFolder()
       || it.current() == m_listView->rootItem()) {
         continue;
      }
      if (it.current()->childCount() == 0) {
         // non folder case
         bookmarks.append(it.current()->bookmark());
         continue;
      }
      QListViewItem *endOfFolder 
         = it.current()->nextSibling() ? it.current()->nextSibling()->itemAbove() : 0;
      QListViewItemIterator it2((QListViewItem*)it.current());
      QListViewItem *last = 0;
      for( ; it2.current() && (last != endOfFolder); (last = it2.current()), it2++) {
         KEBListViewItem *item = static_cast<KEBListViewItem *>(it2.current());
         if (!item->isEmptyFolder() && (item->childCount() == 0)) {
            bookmarks.append(item->bookmark());
         }
      }
   }
   return bookmarks;
}

QValueList<KBookmark> ListView::allBookmarks() {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); it.current() != 0; ++it) {
      if ((it.current()->childCount() == 0) && !it.current()->isEmptyFolder()) {
         bookmarks.append(it.current()->bookmark());
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
   }

   KEBListViewItem *item = firstSelected();
   if (item->isEmptyFolder()) {
      item = static_cast<KEBListViewItem*>(item->parent());
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
   // for the moment listview(1) is all that matters
   m_listView->setCurrentItem(item);
   m_listView->ensureItemVisible(item);
}

KEBListViewItem* ListView::getItemAtAddress(const QString &address) {
   QListViewItem *item = m_listView->rootItem();

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
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); it.current() != 0; ++it) {
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
      sa.singleSelect   = (!sa.multiSelect && sa.itemSelected);
      sa.root           = (m_listView->rootItem() == item);
      sa.multiSelect    = (selectedItems()->count() > 1);
   }

   sa.notEmpty = (m_listView->rootItem()->childCount() > 0);

   return sa;
}

void ListView::handleDropped(KEBListView *lv, QDropEvent *e, QListViewItem *newParent, QListViewItem *itemAfterQLVI) {
   bool inApp = (e->source() == m_listView->viewport())
             || (m_folderListView && e->source() == m_folderListView->viewport());
   bool toOther = e->source() != lv->viewport();

   Q_UNUSED(toOther);

   if (m_splitView) {
      return;
   }

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

   if (!inApp) {
      mcmd = CmdGen::self()->insertMimeSource(i18n("Drop items"), e, newAddress);

   } else {
      QPtrList<KEBListViewItem> *selection = selectedItems();
      KEBListViewItem *firstItem = selection->first();
      if (!firstItem || firstItem == itemAfterQLVI) {
         return;
      }
      bool copy = (e->action() == QDropEvent::Copy);
      mcmd = CmdGen::self()->itemsMoved(selection, newAddress, copy);
   }

   CmdHistory::self()->didCommand(mcmd);
}

static QStringList selected_addresses;

void ListView::updateListView() {
   selected_addresses.clear();
   QPtrList<KEBListViewItem> *selcItems = selectedItems();
   for (QPtrListIterator<KEBListViewItem> it(*selcItems); it.current() != 0; ++it) {
      if (it.current()->bookmark().hasParent()) {
         selected_addresses << it.current()->bookmark().address();
      }
   }
   updateTree();
   if (selectedItems()->count() == 0) {
      m_listView->setSelected(m_listView->currentItem(), true);
   }
}

void ListView::updateTree() {
   fillWithGroup(m_listView, CurrentMgr::self()->mgr()->root());
   if (m_splitView) {
      fillWithGroup(m_folderListView, CurrentMgr::self()->mgr()->root());
   }
}

void ListView::fillWithGroup(KEBListView *lv, KBookmarkGroup group, KEBListViewItem *parentItem) {
   if (!parentItem) {
      lv->clear();
      if (!m_splitView || lv->isFolderList()) {
         KEBListViewItem *tree = new KEBListViewItem(lv, group);
         fillWithGroup(lv, group, tree);
         tree->QListViewItem::setOpen(true);
         return;
      }
   }
   KEBListViewItem *lastItem = 0;
   for (KBookmark bk = group.first(); !bk.isNull(); bk = group.next(bk)) {
      KEBListViewItem *item = 0;
      if (bk.isGroup()) {
         if (m_splitView && !lv->isFolderList()) {
            continue;
         }
         KBookmarkGroup grp = bk.toGroup();
         item = (parentItem)
              ? new KEBListViewItem(parentItem, lastItem, grp)
              : new KEBListViewItem(lv, lastItem, grp);
         fillWithGroup(lv, grp, item);
         if (grp.isOpen()) {
            item->QListViewItem::setOpen(true);
         }
         if (!m_splitView && grp.first().isNull()) {
            // empty folder
            new KEBListViewItem(item, item); 
         }
         lastItem = item;

      } else if (!(lv->isFolderList() && m_splitView)) {
         item = (parentItem)   
              ? ( (lastItem)
                 ? new KEBListViewItem(parentItem, lastItem, bk)
                 : new KEBListViewItem(parentItem, bk))
              : ( (lastItem)
                 ? new KEBListViewItem(lv, lastItem, bk)
                 : new KEBListViewItem(lv, bk));
         lastItem = item;
      }
      QString addr = CurrentMgr::self()->correctAddress(m_last_selection_address);
      if (bk.address() == addr) {
         setCurrent(item);
      }
      if (selected_addresses.contains(bk.address())) {
         lv->setSelected(item, true);
      }
   }
}

void ListView::handleCurrentChanged(KEBListView *, QListViewItem *item) {
   KEBListViewItem *currentItem = static_cast<KEBListViewItem *>(item);
   // hasParent is paranoid, after some thinking remove it
   if (currentItem && currentItem->bookmark().hasParent()) {
      m_last_selection_address = currentItem->bookmark().address();
   }
}

void ListView::handleSelectionChanged(KEBListView *) {
   listview_is_dirty = true;
   KEBApp::self()->updateActions();
   updateSelectedItems();
}

void ListView::handleContextMenu(KEBListView *, KListView *, QListViewItem *qitem, const QPoint &p) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   if (!item) {
      return;
   }
   const char *type = ( (item == m_listView->rootItem()) 
                     || (item->bookmark().isGroup()) 
                     || (item->isEmptyFolder()))
                      ? "popup_folder" : "popup_bookmark";
   QWidget* popup = KEBApp::self()->popupMenuFactory(type);
   if (popup) {
      static_cast<QPopupMenu*>(popup)->popup(p);
   }
}

void ListView::handleDoubleClicked(KEBListView *lv, QListViewItem *item, const QPoint &, int column) {
   lv->rename(item, column);
}

void ListView::handleItemRenamed(KEBListView *lv, QListViewItem *item, const QString &newText, int column) {
   Q_ASSERT(item);
   KBookmark bk = static_cast<KEBListViewItem *>(item)->bookmark();
   KCommand *cmd = 0;
   if (column == KEBListView::NameColumn) {
      if (newText.isEmpty()) {
         // can't have an empty name, therefore undo the user action
         item->setText(KEBListView::NameColumn, bk.fullText());
      } else if (bk.fullText() != newText) {
         cmd = new NodeEditCommand(bk.address(), newText, "title");
      }

   } else if (column == KEBListView::UrlColumn && !lv->isFolderList()) {
      if (bk.url() != newText) {
         cmd = new EditCommand(bk.address(), EditCommand::Edition("href", newText), i18n("URL"));
      }

   } else if (column == KEBListView::CommentColumn && !lv->isFolderList()) {
      if (NodeEditCommand::getNodeText(bk, "desc") != newText) {
         cmd = new NodeEditCommand(bk.address(), newText, "desc");
      }
   }
   CmdHistory::self()->addCommand(cmd);
}

// used by f2 and f3 shortcut slots - see actionsimpl
void ListView::rename(int column) {
   // TODO - check which listview has focus
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
   // this needs to take special care
   // of the current listview focus!
   // but for the moment we just default
   // to using the item listview
   // in fact, because the two are so 
   // different they each need to be 
   // handled almost completely differently...
   KEBListView *lv = m_listView;
   while (1) {
      if (fwd && myrenamecolumn < KEBListView::CommentColumn) {
         myrenamecolumn++;
      } else if (!fwd && myrenamecolumn > KEBListView::NameColumn) {
         myrenamecolumn--;
      } else {
         myrenameitem    = 
            static_cast<KEBListViewItem *>(
              fwd ? ( myrenameitem->itemBelow() 
                    ? myrenameitem->itemBelow() : lv->firstChild() ) 
                  : ( myrenameitem->itemAbove()
                    ? myrenameitem->itemAbove() : lv->lastItem() ) );
         myrenamecolumn  
            = fwd ? KEBListView::NameColumn 
                  : KEBListView::CommentColumn;
      }
      if (!myrenameitem 
        || myrenameitem == m_listView->rootItem()
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
   lv->rename(myrenameitem, myrenamecolumn);
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

KEBListViewItem* KEBListView::rootItem() {
   return static_cast<KEBListViewItem *>(firstChild());
}

QPtrList<KEBListViewItem>* KEBListView::itemList() {
   QPtrList<KEBListViewItem> *items = new QPtrList<KEBListViewItem>();
   for (QListViewItemIterator it(this); it.current(); it++) {
      items->append(static_cast<KEBListViewItem *>(it.current()));
   }
   return items;
}

bool KEBListView::acceptDrag(QDropEvent * e) const {
   return (e->source() == viewport() || KBookmarkDrag::canDecode(e));
}

QDragObject *KEBListView::dragObject() {
   QPtrList<KEBListViewItem> *selcItems = ListView::self()->selectedItems();
   if (selcItems->count() == 0 || selcItems->first()->isEmptyFolder()) {
      // we handle empty folders here as a special
      // case for drag & drop in order to allow 
      // for pasting into a "empty folder"
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
   setText(KEBListView::CommentColumn, NodeEditCommand::getNodeText(bk, "desc"));
   setPixmap(0, SmallIcon(bk.icon()));
   // DESIGN - modUpdate badly needs a redesign
   modUpdate();
}

// DESIGN - following constructors should be names classes or else just explicit

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmarkGroup &gp)
   : QListViewItem(parent, i18n("Bookmarks")), m_bookmark(gp), m_emptyFolder(false) {

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

// root bookmark (first of its group)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark & bk)
   : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolder(false) {

   normalConstruct(bk);
}

// root  bookmark (after another)
KEBListViewItem::KEBListViewItem(QListView *parent, QListViewItem *after, const KBookmark &bk)
   : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolder(false) {
   normalConstruct(bk);
}

// DESIGN - move this into kbookmark or into a helper
void KEBListViewItem::setOpen(bool open) {
   m_bookmark.internalElement().setAttribute("folded", open ? "no" : "yes");
   QListViewItem::setOpen(open);
}

void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &ocg, int col, int w, int a) {
   QColorGroup cg(ocg);

   if (col == KEBListView::StatusColumn) {
      switch (m_paintStyle) {
         case KEBListViewItem::TempStyle: 
         {
            int h, s, v;
            cg.background().hsv(&h,&s,&v);
            QColor color = (v > 180 && v < 220) ? (Qt::darkGray) : (Qt::gray);
            cg.setColor(QColorGroup::Text, color);
            break;
         }
         case KEBListViewItem::BoldStyle:
         {
            QFont font = p->font();
            font.setBold(true);
            p->setFont(font);
            break;
         }
         case KEBListViewItem::DefaultStyle:
            break;
      }
   }

   QListViewItem::paintCell(p, cg, col, w,a);
}

#include "listview.moc"
