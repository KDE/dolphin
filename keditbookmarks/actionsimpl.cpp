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
#include "favicons.h"
#include "testlink.h"
#include "search.h"
#include "listview.h"
#include "mymanager.h"

#include "actionsimpl.h"

void ActionsImpl::slotExpandAll() {
   KEBApp::self()->setAllOpen(true);
}

void ActionsImpl::slotCollapseAll() {
   KEBApp::self()->setAllOpen(false);
}

ActionsImpl* ActionsImpl::s_self = 0;

/* -------------------------------------- */

void ActionsImpl::slotCut() {
   slotCopy();
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Cut Items"), ListView::self()->selectedItems());
   KEBApp::self()->didCommand(mcmd);
}

void ActionsImpl::slotCopy() {
   // this is not a command, because it can't be undone
   Q_ASSERT(ListView::self()->selectedItems()->count() != 0);
   QValueList<KBookmark> bookmarks = ListView::self()->itemsToBookmarks(ListView::self()->selectedItems());
   KBookmarkDrag* data = KBookmarkDrag::newDrag(bookmarks, 0 /* not this ! */);
   kapp->clipboard()->setData(data, QClipboard::Clipboard);
}

void ActionsImpl::slotPaste() {
   KMacroCommand *mcmd = 
      CmdGen::self()->insertMimeSource(
           i18n("Paste"), 
           kapp->clipboard()->data(QClipboard::Clipboard), 
           ListView::self()->userAddress());
   KEBApp::self()->didCommand(mcmd);
}

/* -------------------------------------- */

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
                              ListView::self()->userAddress(),
                              dlg.text(), "bookmark_folder", /*open*/ true);
   KEBApp::self()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark() {
   // TODO - make a setCurrentItem(Command *) which uses finaladdress interface
   CreateCommand * cmd = new CreateCommand(
                               ListView::self()->userAddress(),
                               QString::null, "bookmark", KURL("http://"));
   KEBApp::self()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator() {
   CreateCommand * cmd = new CreateCommand(ListView::self()->userAddress());
   KEBApp::self()->addCommand(cmd);
}

void ActionsImpl::slotImport() { 
   ImportCommand* import = ImportCommand::performImport(sender()->name()+6, KEBApp::self());
   if (!import) {
      return;
   }
   KEBApp::self()->addCommand(import);
   KEBListViewItem *item = ListView::self()->getItemAtAddress(import->groupAddress());
   if (item) {
      ListView::self()->setCurrent(item);
   }
}

void ActionsImpl::slotExportNS() {
   CurrentMgr::self()->doExport(false);
}

void ActionsImpl::slotExportMoz() {
   CurrentMgr::self()->doExport(true);
}

/* -------------------------------------- */

void ActionsImpl::slotShowNS() {
   bool shown = KEBApp::self()->nsShown();
   CurrentMgr::self()->mgr()->setShowNSBookmarks(shown);
   KEBApp::self()->setModifiedFlag(true);
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
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(ListView::self()->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(ListView::self()->allBookmarks()));
}

/* -------------------------------------- */

void ActionsImpl::slotTestSelection() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(ListView::self()->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(ListView::self()->selectedBookmarksExpanded()));
}

void ActionsImpl::slotSearch() {
   // TODO
   // also, need to think about limiting size of itr list to <= 1
   // or, generically. itr's shouldn't overlap. difficult problem...
   bool ok;
   QString text = KLineEditDlg::getText("Find string in bookmarks:", "", &ok, KEBApp::self());
   SearchItr* itr = new SearchItr(ListView::self()->allBookmarks());
   itr->setText(text);
   SearchItrHolder::self()->insertItr(itr);
}

void ActionsImpl::slotNextHit() {
   SearchItrHolder::self()->nextOne();
}

/* -------------------------------------- */

void ActionsImpl::slotSort() {
   KBookmark bk = ListView::self()->firstSelected()->bookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   KEBApp::self()->addCommand(cmd);
}

/* -------------------------------------- */

void ActionsImpl::slotDelete() {
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), ListView::self()->selectedItems());
   KEBApp::self()->didCommand(mcmd);
}

void ActionsImpl::slotOpenLink() {
   QValueList<KBookmark> bks = ListView::self()->itemsToBookmarks(ListView::self()->selectedItems());
   QValueListIterator<KBookmark> it;
   for (it = bks.begin(); it != bks.end(); ++it) {
      if ((*it).isGroup() || (*it).isSeparator()) {
         continue;
      }
      (void)new KRun((*it).url());
   }
}

/* -------------------------------------- */

void ActionsImpl::slotRename() {
   ListView::self()->rename(KEBListView::NameColumn);
}

void ActionsImpl::slotChangeURL() {
   ListView::self()->rename(KEBListView::UrlColumn);
}

void ActionsImpl::slotChangeComment() {
   ListView::self()->rename(KEBListView::CommentColumn);
}

void ActionsImpl::slotSetAsToolbar() {
   KBookmark bk = ListView::self()->firstSelected()->bookmark();
   Q_ASSERT(bk.isGroup());
   KMacroCommand *mcmd = CmdGen::self()->setAsToolbar(bk);
   KEBApp::self()->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
   KBookmark bk = ListView::self()->firstSelected()->bookmark();
   KIconDialog dlg(KEBApp::self());
   QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
   if (newIcon.isEmpty()) {
      return;
   }
   EditCommand *cmd = new EditCommand(
                            bk.address(),
                            EditCommand::Edition("icon", newIcon),
                            i18n("Icon"));
   KEBApp::self()->addCommand(cmd);
}

#include "actionsimpl.moc"
