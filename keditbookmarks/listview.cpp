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

#include "listview.h"

#include "toplevel.h"
#include "commands.h"
#include "testlink.h" // paintCellHelper

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

// #define DEBUG_ADDRESSES

ListView* ListView::s_self = 0;

int ListView::s_myrenamecolumn = -1;
KEBListViewItem *ListView::s_myrenameitem = 0;

QStringList ListView::s_selected_addresses; // UGLY
bool ListView::s_listview_is_dirty = false;

ListView::ListView() {
   m_splitView = KEBApp::self()->splitView();
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

void ListView::updateListViewSetup(bool readonly) {
   self()->m_listView->readonlyFlagInit(readonly);
   if (m_splitView) {
      self()->m_folderListView->readonlyFlagInit(readonly);
   }
}

void ListView::setInitialAddress(QString address) {
   m_last_selection_address = address;
}

void ListView::connectSignals() {
   m_listView->makeConnections();
   if (m_splitView) {
      m_folderListView->makeConnections();
   }
}

QValueList<KBookmark> ListView::itemsToBookmarks(QPtrList<KEBListViewItem>* items) const {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*items); it.current() != 0; ++it) {
      bookmarks.append(KBookmark(it.current()->bookmark()));
   }
   return bookmarks;
}

QPtrList<KEBListViewItem>* ListView::selectedItems() const {
   static QPtrList<KEBListViewItem>* s_selected_items_cache = 0;
   if (!s_selected_items_cache || s_listview_is_dirty) {
      QPtrList<KEBListViewItem> *items = new QPtrList<KEBListViewItem>();
      for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList()));
           it.current() != 0; ++it) {
         if (it.current()->isSelected()) {
            items->append(it.current());
         }
      }
      s_selected_items_cache = items;
   }
   return s_selected_items_cache;
}

KEBListViewItem* ListView::firstSelected() const {
   return selectedItems()->first();
}

KEBListViewItem* ListView::findOpenParent(KEBListViewItem *item) const {
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

ListView::Which ListView::whichChildrenSelected(KEBListViewItem *item) {
   bool some = false;
   bool all = true;
   QListViewItem *endOfFolder
      = item->nextSibling() ? item->nextSibling()->itemAbove() : 0;
   QListViewItemIterator it((QListViewItem*)item);
   it++;
   QListViewItem *last = 0;
   for( ; it.current() && (last != endOfFolder); (last = it.current()), it++) {
      KEBListViewItem *item = static_cast<KEBListViewItem *>(it.current());
      if (!item->isEmptyFolderPadder()) {
         if (item->isSelected()) {
            some = true;
         } else {
            all = false;
         }
      }
   }
   return all ? AllChildren : (some ? SomeChildren : NoChildren);
}

void ListView::deselectAllButParent(KEBListViewItem *item) {
   QListViewItem *endOfFolder
      = item->nextSibling() ? item->nextSibling()->itemAbove() : 0;
   QListViewItemIterator it((QListViewItem*)item);
   it++;
   QListViewItem *last = 0;
   for( ; it.current() && (last != endOfFolder); (last = it.current()), it++) {
      KEBListViewItem *item = static_cast<KEBListViewItem *>(it.current());
      if (!item->isEmptyFolderPadder() && item->isSelected()) {
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
      if (!it.current()->isEmptyFolderPadder() && it.current()->isSelected()) {
         selected = true;
      } else {
         continue;
      }
      if (it.current()->childCount() == 0) {
         // don't bother looking into it if its not a folder
         continue;
      }
      Which which = whichChildrenSelected(it.current());
      if (which == AllChildren) { 
         // select outer folder
         deselectAllButParent(it.current());
      } else if (which == SomeChildren) { 
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
      if (it.current()->isEmptyFolderPadder()) {
         it.current()->setSelected(false);
      }
   }
}

QValueList<KBookmark> ListView::selectedBookmarksExpanded() const {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); it.current() != 0; ++it) {
      if (!it.current()->isSelected() 
       || it.current()->isEmptyFolderPadder()
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
         if (!item->isEmptyFolderPadder() && (item->childCount() == 0)) {
            bookmarks.append(item->bookmark());
         }
      }
   }
   return bookmarks;
}

QValueList<KBookmark> ListView::allBookmarks() const {
   QValueList<KBookmark> bookmarks;
   for (QPtrListIterator<KEBListViewItem> it(*(m_listView->itemList())); it.current() != 0; ++it) {
      if ((it.current()->childCount() == 0) && !it.current()->isEmptyFolderPadder()) {
         bookmarks.append(it.current()->bookmark());
      }
   }
   return bookmarks;
}

// DESIGN - make + "/0" a kbookmark:: thing?

QString ListView::userAddress() const {
   if (selectedItems()->isEmpty()) {
      // FIXME - maybe a in view one?
      //       - else we could get /0
      //       - in view?
      return "/0";
   }

   KEBListViewItem *item = firstSelected();
   if (item->isEmptyFolderPadder()) {
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

KEBListViewItem* ListView::getItemAtAddress(const QString &address) const {
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

SelcAbilities ListView::getSelectionAbilities() const {
   KEBListViewItem *item = firstSelected();

   static SelcAbilities sa = { false, false, false, false, false, false, false, false, false };

   if (item) {
      KBookmark nbk = item->bookmark();
      sa.itemSelected   = true;
      sa.group          = nbk.isGroup();
      sa.separator      = nbk.isSeparator();
      sa.urlIsEmpty     = nbk.url().isEmpty();
      sa.singleSelect   = (!sa.multiSelect && sa.itemSelected);
      sa.root           = (m_listView->rootItem() == item);
      sa.multiSelect    = (selectedItems()->count() > 1);
      sa.tbShowState    = CmdGen::self()->shownInToolbar(nbk);
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
      = (!itemAfter || itemAfter->isEmptyFolderPadder())
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

void ListView::updateListView() {
   s_selected_addresses.clear();
   QPtrList<KEBListViewItem> *selcItems = selectedItems();
   for (QPtrListIterator<KEBListViewItem> it(*selcItems); it.current() != 0; ++it) {
      if (it.current()->bookmark().hasParent()) {
         s_selected_addresses << it.current()->bookmark().address();
      }
   }
   int lastCurrentY = m_listView->contentsY();
   updateTree();
   if (selectedItems()->isEmpty())
      m_listView->setSelected(m_listView->currentItem(), true);
   m_listView->ensureVisible(0, lastCurrentY, 0, 0);
   m_listView->ensureVisible(0, lastCurrentY + m_listView->visibleHeight(), 0, 0);
}

void ListView::updateTree(bool updateSplitView) {
   KBookmarkGroup root = CurrentMgr::self()->mgr()->root();
   if (m_splitView) {
      root = CurrentMgr::bookmarkAt(m_currentSelectedRootAddress).toGroup();
   }
   fillWithGroup(m_listView, root);
   if (m_splitView && updateSplitView) {
      fillWithGroup(m_folderListView, CurrentMgr::self()->mgr()->root());
   }
}

void ListView::fillWithGroup(KEBListView *lv, KBookmarkGroup group, KEBListViewItem *parentItem) {
   KEBListViewItem *lastItem = 0;
   if (!parentItem) {
      lv->clear();
      if (!m_splitView || lv->isFolderList()) {
         KEBListViewItem *tree = new KEBListViewItem(lv, group);
         fillWithGroup(lv, group, tree);
         tree->QListViewItem::setOpen(true);
         return;
      }
   }
   if (m_splitView && !lv->isFolderList()) {
      lastItem = new KEBListViewItem(lv, lastItem, group);
   }
   for (KBookmark bk = group.first(); !bk.isNull(); bk = group.next(bk)) {
      KEBListViewItem *item = 0;
      if (bk.isGroup()) {
         KBookmarkGroup grp = bk.toGroup();
         item = (parentItem)
              ? new KEBListViewItem(parentItem, lastItem, grp)
              : new KEBListViewItem(lv, lastItem, grp);
         if (!(m_splitView && !lv->isFolderList())) {
            fillWithGroup(lv, grp, item);
            if (grp.isOpen()) {
               item->QListViewItem::setOpen(true);
            }
            if (!m_splitView && grp.first().isNull()) {
               // empty folder
               new KEBListViewItem(item, item); 
            }
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
      if (bk.address() == addr)
         setCurrent(item);
      if (s_selected_addresses.contains(bk.address()))
         lv->setSelected(item, true);
   }
}

void ListView::handleCurrentChanged(KEBListView *lv, QListViewItem *item) {
   // hasParent is paranoid, after some thinking remove it
   KEBListViewItem *currentItem = static_cast<KEBListViewItem *>(item);
   if (currentItem && currentItem->bookmark().hasParent())
      m_last_selection_address = selectedItems()->count() >= 1
                               ? selectedItems()->first()->bookmark().address()
                               : currentItem->bookmark().address();
   if (item && m_splitView && lv == m_folderListView) {
      m_folderListView->setSelected(item, true);
      QString addr = currentItem->bookmark().address();
      if (addr != m_currentSelectedRootAddress) {
         m_currentSelectedRootAddress = addr;
         updateTree(false);
      }
   }
}

void ListView::handleMoved(KEBListView *) {
   kdDebug() << "ListView::handleMoved()" << endl;  
   // TODO ERM WTF TODO
   /*
   KMacroCommand *mcmd = CmdGen::self()->deleteItems( i18n("Moved Items"), 
                                                      ListView::self()->selectedItems());
   CmdHistory::self()->didCommand(mcmd);
   */
}

void ListView::handleSelectionChanged(KEBListView *) {
   s_listview_is_dirty = true;
   KEBApp::self()->updateActions();
   updateSelectedItems();
   if (selectedItems()->count() >= 1) {
      KEBApp::self()->bkInfo()->showBookmark(firstSelected()->bookmark());
   }
}

void ListView::handleContextMenu(KEBListView *, KListView *, QListViewItem *qitem, const QPoint &p) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   const char *type = ( !item
                     || (item == m_listView->rootItem()) 
                     || (item->bookmark().isGroup()) 
                     || (item->isEmptyFolderPadder()))
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

void ListView::startRename(int column, KEBListViewItem *item) {
   s_myrenamecolumn = column;
   s_myrenameitem = item;
}

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
      if (fwd && s_myrenamecolumn < KEBListView::CommentColumn) {
         s_myrenamecolumn++;
      } else if (!fwd && s_myrenamecolumn > KEBListView::NameColumn) {
         s_myrenamecolumn--;
      } else {
         s_myrenameitem    = 
            static_cast<KEBListViewItem *>(
              fwd ? ( s_myrenameitem->itemBelow() 
                    ? s_myrenameitem->itemBelow() : lv->firstChild() ) 
                  : ( s_myrenameitem->itemAbove()
                    ? s_myrenameitem->itemAbove() : lv->lastItem() ) );
         s_myrenamecolumn  
            = fwd ? KEBListView::NameColumn 
                  : KEBListView::CommentColumn;
      }
      if (s_myrenameitem 
       && s_myrenameitem != m_listView->rootItem()
       && !s_myrenameitem->isEmptyFolderPadder()
       && !s_myrenameitem->bookmark().isSeparator()
       && !(s_myrenamecolumn == KEBListView::UrlColumn && s_myrenameitem->bookmark().isGroup())
      ) {
         break;
      }
   }
   lv->rename(s_myrenameitem, s_myrenamecolumn);
}

/* -------------------------------------- */

class KeyPressEater : public QObject {
public:
   KeyPressEater( QWidget *parent = 0, const char *name = 0 ) { 
      m_allowedToTab = true; 
   }
protected:
   bool eventFilter(QObject *, QEvent *);
   bool m_allowedToTab;
};

bool KeyPressEater::eventFilter(QObject *, QEvent *pe) {
   if (pe->type() == QEvent::KeyPress) {
      QKeyEvent *k = (QKeyEvent *) pe;
      if ((k->key() == Qt::Key_Backtab || k->key() == Qt::Key_Tab)
      && !(k->state() & ControlButton || k->state() & AltButton)
      ) {
         if (m_allowedToTab) {
            bool fwd = (k->key() == Key_Tab && !(k->state() & ShiftButton));
            ListView::self()->renameNextCell(fwd);
         }
         return true;
      } else {
         m_allowedToTab = (k->key() == Qt::Key_Escape || k->key() == Qt::Key_Enter);
      }
   }
   return false;
}

/* -------------------------------------- */

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

void KEBListView::makeConnections() {
   connect(this, SIGNAL( moved() ),
                 SLOT( slotMoved() ));
   connect(this, SIGNAL( selectionChanged() ),
                 SLOT( slotSelectionChanged() ));
   connect(this, SIGNAL( currentChanged(QListViewItem *) ),
                 SLOT( slotCurrentChanged(QListViewItem *) ));
   connect(this, SIGNAL( contextMenu(KListView *, QListViewItem*, const QPoint &) ),
                 SLOT( slotContextMenu(KListView *, QListViewItem *, const QPoint &) ));
   connect(this, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
                 SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ));
   connect(this, SIGNAL( doubleClicked(QListViewItem *, const QPoint &, int) ),
                 SLOT( slotDoubleClicked(QListViewItem *, const QPoint &, int) ));
   connect(this, SIGNAL( dropped(QDropEvent*, QListViewItem*, QListViewItem*) ),
                 SLOT( slotDropped(QDropEvent*, QListViewItem*, QListViewItem*) ));
}

void KEBListView::readonlyFlagInit(bool readonly) {
   setItemsMovable(readonly); // we move items ourselves (for undo)
   setItemsRenameable(!readonly);
   setAcceptDrops(!readonly);
   setDropVisualizer(!readonly);
}

void KEBListView::slotMoved() 
   { ListView::self()->handleMoved(this); }
void KEBListView::slotSelectionChanged() 
   { ListView::self()->handleSelectionChanged(this); }
void KEBListView::slotCurrentChanged(QListViewItem *a) 
   { ListView::self()->handleCurrentChanged(this, a); }
void KEBListView::slotContextMenu(KListView *a, QListViewItem *b, const QPoint &c) 
   { ListView::self()->handleContextMenu(this, a,b,c); }
void KEBListView::slotItemRenamed(QListViewItem *a, const QString &b, int c) 
   { ListView::self()->handleItemRenamed(this, a,b,c); }
void KEBListView::slotDoubleClicked(QListViewItem *a, const QPoint &b, int c) 
   { ListView::self()->handleDoubleClicked(this, a,b,c); }
void KEBListView::slotDropped(QDropEvent *a, QListViewItem *b, QListViewItem *c) 
   { ListView::self()->handleDropped(this, a,b,c); }

void KEBListView::rename(QListViewItem *qitem, int column) {
   KEBListViewItem *item = static_cast<KEBListViewItem *>(qitem);
   if ( !(column == NameColumn || column == UrlColumn || column == CommentColumn)
     || KEBApp::self()->readonly()
     || !item 
     || item == firstChild() 
     || item->isEmptyFolderPadder()
     || item->bookmark().isSeparator()
     || (column == UrlColumn && item->bookmark().isGroup())
   ) {
      return;
   }
   ListView::startRename(column, item);
   KeyPressEater *keyPressEater = new KeyPressEater(this);
   renameLineEdit()->installEventFilter(keyPressEater);
   KListView::rename(item, column);
}

KEBListViewItem* KEBListView::rootItem() const {
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
   if (selcItems->isEmpty() || selcItems->first()->isEmptyFolderPadder()) {
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
   bool shown = CmdGen::self()->shownInToolbar(bk);
   setPixmap(0, SmallIcon(shown ? "bookmark_toolbar" : bk.icon()));
   // DESIGN - modUpdate badly needs a redesign
   modUpdate();
}

// DESIGN - following constructors should be names classes or else just explicit

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmarkGroup &gp)
   : QListViewItem(parent, KEBApp::self()->caption().isNull() 
                         ? i18n("Bookmarks")
                         : i18n("%1 Bookmarks").arg(KEBApp::self()->caption())), 
     m_bookmark(gp), m_emptyFolderPadder(false) {

   setPixmap(0, SmallIcon("bookmark"));
   setExpandable(true);
}

// empty folder item
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after)
    : QListViewItem(parent, after, i18n("Empty Folder") ), m_emptyFolderPadder(true) {

   setPixmap(0, SmallIcon("bookmark"));
}

// group
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup &gp)
   : QListViewItem(parent, after, gp.fullText()), m_bookmark(gp), m_emptyFolderPadder(false) {

   setExpandable(true);
   normalConstruct(gp);
}

// bookmark (first of its group)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk)
   : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolderPadder(false) {

   normalConstruct(bk);
}

// bookmark (after another)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark &bk)
   : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolderPadder(false) {
   normalConstruct(bk);
}

// root bookmark (first of its group)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark & bk)
   : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolderPadder(false) {

   normalConstruct(bk);
}

// root  bookmark (after another)
KEBListViewItem::KEBListViewItem(QListView *parent, QListViewItem *after, const KBookmark &bk)
   : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk), m_emptyFolderPadder(false) {
   normalConstruct(bk);
}

// DESIGN - move this into kbookmark or into a helper
void KEBListViewItem::setOpen(bool open) {
   if (!parent())
      return;
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
