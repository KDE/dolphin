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
#include <kbookmarkexporter.h>
#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include <klineeditdlg.h>

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

/* ------------------------------------------------------------- */
//                             CLIPBOARD
/* ------------------------------------------------------------- */

void KEBTopLevel::slotClipboardDataChanged() {
   kdDebug() << "KEBTopLevel::slotClipboardDataChanged" << endl;
   if (!m_bReadOnly) {
      m_bCanPaste = KBookmarkDrag::canDecode(KEBClipboard::get());
      ListView::self()->emitSlotSelectionChanged();
   }
}

void KEBTopLevel::slotCut() {
   slotCopy();
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Cut Items"), listview->selectedItems());
   didCommand(mcmd);
}

void KEBTopLevel::slotCopy() {
   // this is not a command, because it can't be undone
   Q_ASSERT(listview->selectedItems()->count() != 0);
   QValueList<KBookmark> bookmarks = listview->itemsToBookmarks(listview->selectedItems());
   KBookmarkDrag* data = KBookmarkDrag::newDrag(bookmarks, 0 /* not this ! */);
   KEBClipboard::set(data);
}

void KEBTopLevel::slotPaste() {
   KMacroCommand *mcmd = CmdGen::self()->insertMimeSource(i18n("Paste"), KEBClipboard::get(), listview->userAddress());
   didCommand(mcmd);
}

/* ------------------------------------------------------------- */
//                             POS_ACTION
/* ------------------------------------------------------------- */

void KEBTopLevel::slotNewFolder() {
   // TODO - move this into a sanely named gui class in kio/bookmarks?
   KLineEditDlg dlg(i18n("New folder:"), "", 0);
   dlg.setCaption(i18n("Create New Bookmark Folder"));
   // text is empty by default, therefore disable ok button.
   dlg.enableButtonOK(false);
   if (dlg.exec()) {
      CreateCommand *cmd = new CreateCommand(
                              listview->userAddress(), 
                              dlg.text(), "bookmark_folder", /*open*/ true);
      addCommand(cmd);
   }
}

void KEBTopLevel::slotNewBookmark() {
   CreateCommand * cmd = new CreateCommand( 
                                listview->userAddress(), 
                                QString::null, QString::null, KURL());
   addCommand(cmd);
}

void KEBTopLevel::slotInsertSeparator() {
   CreateCommand * cmd = new CreateCommand(listview->userAddress());
   addCommand(cmd);
}

#define IMPL(a) addImport(callImporter<a>(this));

void KEBTopLevel::slotImportIE()     { IMPL(IEImportCommand);     }
void KEBTopLevel::slotImportGaleon() { IMPL(GaleonImportCommand); }
void KEBTopLevel::slotImportKDE()    { IMPL(KDE2ImportCommand);   }
void KEBTopLevel::slotImportOpera()  { IMPL(OperaImportCommand);  }
void KEBTopLevel::slotImportMoz()    { IMPL(MozImportCommand);    }
void KEBTopLevel::slotImportNS()     { IMPL(NSImportCommand);     }

void KEBTopLevel::slotExportNS() {
   myManager()->doExport(KNSBookmarkImporter::netscapeBookmarksFile(true), false);
}

void KEBTopLevel::slotExportMoz() {
   myManager()->doExport(KNSBookmarkImporter::mozillaBookmarksFile(true), true);
}

/* ------------------------------------------------------------- */
//                            NULL_ACTION
/* ------------------------------------------------------------- */

// TOPLEVEL???
void KEBTopLevel::slotSaveOnClose() {
   m_saveOnClose = static_cast<KToggleAction*>(actionCollection()->action("settings_saveonclose"))->isChecked();
}

void KEBTopLevel::slotShowNS() {
   // one will need to save, to get konq to notice the change
   // if that's bad, then we need to put this flag in a KConfig.
   myManager()->flipShowNSFlag();
   setModifiedFlag(true);
}

void KEBTopLevel::slotTestAll() {
   TestLinkItrHolder::self()->insertItr(
      new TestLinkItr(listview->allBookmarks()));
}

void KEBTopLevel::slotUpdateAllFavIcons() {
   FavIconsItrHolder::self()->insertItr(
      new FavIconsItr(listview->allBookmarks()));
}

void KEBTopLevel::slotCancelFavIconUpdates() {
   FavIconsItrHolder::self()->cancelAllItrs();
}

void KEBTopLevel::slotCancelAllTests() {
   TestLinkItrHolder::self()->cancelAllItrs();
}

void KEBTopLevel::slotCancelSearch() {
   SearchItrHolder::self()->cancelAllItrs();
}

// SHUFFLE- move back into functions themselves, or rename selectedBookmarksExpanded/.*Holder/insertItr to make it obvious.
/* ------------------------------------------------------------- */
//                             ITR_ACTION
/* ------------------------------------------------------------- */

void KEBTopLevel::slotTestSelection() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(listview->selectedBookmarksExpanded()));
}

void KEBTopLevel::slotUpdateFavIcon() {
   FavIconsItrHolder::self()->insertItr(
      new FavIconsItr(listview->selectedBookmarksExpanded()));
}

void KEBTopLevel::slotSearch() {
   // also, need to think about limiting size of itr list to <= 1
   // or, generically. itr's shouldn't overlap. difficult problem...
   bool ok;
   QString text = KLineEditDlg::getText("Find string in bookmarks:", "", &ok, this);
   SearchItr* itr = new SearchItr(listview->allBookmarks());
   itr->setText(text);
   SearchItrHolder::self()->insertItr(itr);
}

/* ------------------------------------------------------------- */
//                            GROUP_ACTION
/* ------------------------------------------------------------- */

void KEBTopLevel::slotSort() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   addCommand(cmd);
}

/* ------------------------------------------------------------- */
//                           SELC_ACTION
/* ------------------------------------------------------------- */

void KEBTopLevel::slotDelete() {
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), listview->selectedItems());
   didCommand(mcmd);
}


void KEBTopLevel::slotOpenLink() {
   QValueList<KBookmark> bks = listview->itemsToBookmarks(listview->selectedItems());
   QValueListIterator<KBookmark> it;
   for (it = bks.begin(); it != bks.end(); ++it) {
      if (!(*it).isGroup()) {
         (void)new KRun((*it).url());
      }
   }
}

/* ------------------------------------------------------------- */
//                          ITEM_ACTION
/* ------------------------------------------------------------- */

void KEBTopLevel::slotRename() {
   QListViewItem *item = listview->firstSelected();
   Q_ASSERT(item);
   listView()->rename(item, COL_NAME);
}

void KEBTopLevel::slotChangeURL() {
   QListViewItem* item = listview->firstSelected();
   Q_ASSERT(item);
   listView()->rename(item, COL_URL);
}

void KEBTopLevel::slotSetAsToolbar() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   KMacroCommand *mcmd = CmdGen::self()->setAsToolbar(bk);
   addCommand(mcmd);
}

void KEBTopLevel::slotChangeIcon() {
   KBookmark bk = listview->selectedBookmark();
   KIconDialog dlg(this);
   QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
   if (!newIcon.isEmpty()) {
      EditCommand *cmd = new EditCommand(
                                 bk.address(),
                                 EditCommand::Edition("icon", newIcon),
                                 i18n("Icon"));
      addCommand(cmd);
   }
}

