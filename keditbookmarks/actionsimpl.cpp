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
#include <kinputdialog.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "favicons.h"
#include "testlink.h"
#include "listview.h"

#include "actionsimpl.h"

ActionsImpl* ActionsImpl::s_self = 0;

void ActionsImpl::slotCut() {
   slotCopy();
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Cut Items"), ListView::self()->selectedItems());
   CmdHistory::self()->didCommand(mcmd);
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
   CmdHistory::self()->didCommand(mcmd);
}

/* -------------------------------------- */

void ActionsImpl::slotNewFolder() {
   bool ok;
   QString str = KInputDialog::getText( i18n( "Create New Bookmark Folder" ),
      i18n( "New folder:" ), QString::null, &ok );
   if (!ok)
      return;

   CreateCommand *cmd = new CreateCommand(
                              ListView::self()->userAddress(),
                              str, "bookmark_folder", /*open*/ true);
   CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark() {
   // TODO - make a setCurrentItem(Command *) which uses finaladdress interface
   CreateCommand * cmd = new CreateCommand(
                               ListView::self()->userAddress(),
                               QString::null, "www", KURL("http://"));
   CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator() {
   CreateCommand * cmd = new CreateCommand(ListView::self()->userAddress());
   CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotImport() { 
   kdDebug() << "ActionsImpl::slotImport() where sender()->name() == " << sender()->name() << endl;
   ImportCommand* import = ImportCommand::performImport(sender()->name()+6, KEBApp::self());
   if (!import) {
      return;
   }
   // TODO - following line doesn't work, fixme
   ListView::self()->setInitialAddress(import->groupAddress());
   CmdHistory::self()->addCommand(import);
}

// TODO - this is getting ugly and repetitive. cleanup!

void ActionsImpl::slotExportOpera() {
   CurrentMgr::self()->doExport(CurrentMgr::OperaExport); }
void ActionsImpl::slotExportHTML() {
   CurrentMgr::self()->doExport(CurrentMgr::HTMLExport); }
void ActionsImpl::slotExportIE() {
   CurrentMgr::self()->doExport(CurrentMgr::IEExport); }
void ActionsImpl::slotExportNS() {
   CurrentMgr::self()->doExport(CurrentMgr::NetscapeExport); }
void ActionsImpl::slotExportMoz() {
   CurrentMgr::self()->doExport(CurrentMgr::MozillaExport); }

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

/* -------------------------------------- */

class KBookmarkGroupList : private KBookmarkGroupTraverser {
public:
   KBookmarkGroupList(KBookmarkManager *);
   QValueList<KBookmark> getList(const KBookmarkGroup &);
private:
   virtual void visit(const KBookmark &) { ; }
   virtual void visitEnter(const KBookmarkGroup &);
   virtual void visitLeave(const KBookmarkGroup &) { ; }
private:
   KBookmarkManager *m_manager;
   QValueList<KBookmark> m_list;
};

KBookmarkGroupList::KBookmarkGroupList( KBookmarkManager *manager ) {
   m_manager = manager;
}

QValueList<KBookmark> KBookmarkGroupList::getList( const KBookmarkGroup &grp ) {
   traverse(grp);
   return m_list;
}

void KBookmarkGroupList::visitEnter(const KBookmarkGroup &grp) {
   m_list << grp;
}

void ActionsImpl::slotRecursiveSort() {
   KBookmark bk = ListView::self()->firstSelected()->bookmark();
   Q_ASSERT(bk.isGroup());
   KMacroCommand *mcmd = new KMacroCommand(i18n("Recursive Sort"));
   KBookmarkGroupList lister(CurrentMgr::self()->mgr());
   QValueList<KBookmark> bookmarks = lister.getList(bk.toGroup());
   bookmarks << bk.toGroup();
   for (QValueListConstIterator<KBookmark> it = bookmarks.begin(); it != bookmarks.end(); ++it) {
      SortCommand *cmd = new SortCommand("", (*it).address());
      cmd->execute();
      mcmd->addCommand(cmd);
   }
   CmdHistory::self()->didCommand(mcmd);
}

void ActionsImpl::slotSort() {
   KBookmark bk = ListView::self()->firstSelected()->bookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   CmdHistory::self()->addCommand(cmd);
}

/* -------------------------------------- */

void ActionsImpl::slotDelete() {
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), ListView::self()->selectedItems());
   CmdHistory::self()->didCommand(mcmd);
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
   CmdHistory::self()->addCommand(mcmd);
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
   CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotExpandAll() {
   ListView::self()->setOpen(true);
   KEBApp::self()->setModifiedFlag(true);
}

void ActionsImpl::slotCollapseAll() {
   ListView::self()->setOpen(false);
   KEBApp::self()->setModifiedFlag(true);
}

#include "actionsimpl.moc"
