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
#include <klineeditdlg.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include "kebbookmarkexporter.h"

// DESIGN - shuffle, sort out includes in general

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "core.h"
#include "favicons.h"
#include "testlink.h"
#include "search.h"
#include "listview.h"
#include "mymanager.h"

#include "actionsimpl.h"

#define top KEBTopLevel::self()

void ActionsImpl::slotExpandAll() {
   top->setAllOpen(true);
}

void ActionsImpl::slotCollapseAll() {
   top->setAllOpen(false);
}

ActionsImpl* ActionsImpl::s_self = 0;

/* ------------------------------------------------------------- */
//                             CLIPBOARD
/* ------------------------------------------------------------- */

void ActionsImpl::slotCut() {
   slotCopy();
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Cut Items"), listview->selectedItems());
   top->didCommand(mcmd);
}

void ActionsImpl::slotCopy() {
   // this is not a command, because it can't be undone
   Q_ASSERT(listview->selectedItems()->count() != 0);
   QValueList<KBookmark> bookmarks = listview->itemsToBookmarks(listview->selectedItems());
   KBookmarkDrag* data = KBookmarkDrag::newDrag(bookmarks, 0 /* not this ! */);
   KEBClipboard::set(data);
}

void ActionsImpl::slotPaste() {
   KMacroCommand *mcmd = CmdGen::self()->insertMimeSource(i18n("Paste"), KEBClipboard::get(), listview->userAddress());
   top->didCommand(mcmd);
}

/* ------------------------------------------------------------- */
//                             POS_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotNewFolder() {
   // TODO - move this into a sanely named gui class in kio/bookmarks?
   KLineEditDlg dlg(i18n("New folder:"), "", 0);
   dlg.setCaption(i18n("Create New Bookmark Folder"));
   // text is empty by default, therefore disable ok button.
   dlg.enableButtonOK(false);
   if (!dlg.exec()) {
      return;
   }
   CreateCommand *cmd = new CreateCommand(
                              listview->userAddress(),
                              dlg.text(), "bookmark_folder", /*open*/ true);
   top->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark() {
   CreateCommand * cmd = new CreateCommand(
                               listview->userAddress(),
                               QString::null, QString::null, KURL());
   top->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator() {
   CreateCommand * cmd = new CreateCommand(listview->userAddress());
   top->addCommand(cmd);
}

void ActionsImpl::slotImport() { 
   top->addImport(ImportCommand::performImport(sender()->name()+6, top));
}

void ActionsImpl::slotExportNS() {
   MyManager::self()->doExport(KNSBookmarkImporter::netscapeBookmarksFile(true), false);
}

void ActionsImpl::slotExportMoz() {
   MyManager::self()->doExport(KNSBookmarkImporter::mozillaBookmarksFile(true), true);
}

/* ------------------------------------------------------------- */
//                            NULL_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotShowNS() {
   bool shown = top->nsShown();
   BkManagerAccessor::mgr()->setShowNSBookmarks(shown);
   top->setModifiedFlag(true);
}

void ActionsImpl::slotCancelFavIconUpdates() {
   FavIconsItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelAllTests() {
   TestLinkItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelSearch() {
   SearchItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotTestAll() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(listview->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(listview->allBookmarks()));
}

/* ------------------------------------------------------------- */
//                             ITR_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotTestSelection() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(listview->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(listview->selectedBookmarksExpanded()));
}

void ActionsImpl::slotSearch() {
   // TODO
   // also, need to think about limiting size of itr list to <= 1
   // or, generically. itr's shouldn't overlap. difficult problem...
   bool ok;
   QString text = KLineEditDlg::getText("Find string in bookmarks:", "", &ok, top);
   SearchItr* itr = new SearchItr(listview->allBookmarks());
   itr->setText(text);
   SearchItrHolder::self()->insertItr(itr);
}

/* ------------------------------------------------------------- */
//                            GROUP_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotSort() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   top->addCommand(cmd);
}

/* ------------------------------------------------------------- */
//                           SELC_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotDelete() {
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), listview->selectedItems());
   top->didCommand(mcmd);
}

void ActionsImpl::slotOpenLink() {
   QValueList<KBookmark> bks = listview->itemsToBookmarks(listview->selectedItems());
   QValueListIterator<KBookmark> it;
   for (it = bks.begin(); it != bks.end(); ++it) {
      if ((*it).isGroup() || (*it).isSeparator()) {
         continue;
      }
      (void)new KRun((*it).url());
   }
}

/* ------------------------------------------------------------- */
//                          ITEM_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotRename() {
   listview->rename(COL_NAME);
}

void ActionsImpl::slotChangeURL() {
   listview->rename(COL_URL);
}

void ActionsImpl::slotSetAsToolbar() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   KMacroCommand *mcmd = CmdGen::self()->setAsToolbar(bk);
   top->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
   KBookmark bk = listview->selectedBookmark();
   KIconDialog dlg(top);
   QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
   if (newIcon.isEmpty()) {
      return;
   }
   EditCommand *cmd = new EditCommand(
                            bk.address(),
                            EditCommand::Edition("icon", newIcon),
                            i18n("Icon"));
   top->addCommand(cmd);
}

#include "actionsimpl.moc"
