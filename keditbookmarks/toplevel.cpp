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

#include <klocale.h>
#include <kdebug.h>

#include <kapplication.h>
#include <kstdaction.h>
#include <kaction.h>

#include <kkeydialog.h>
#include <kedittoolbar.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include "listview.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "mymanager.h"
// DESIGN - inline clipboard stuff?
#include "core.h"
#include <qclipboard.h>

#include "toplevel.h"

#define cmdHistory this

KEBTopLevel *KEBTopLevel::s_topLevel = 0;

KEBTopLevel::KEBTopLevel(const QString & bookmarksFile, bool readonly, QString address)
   : KMainWindow(), m_commandHistory(actionCollection()), m_dcopIface(0) {

   m_bookmarksFilename = bookmarksFile;
   m_readOnly = readonly;
   m_saveOnClose = true;

   s_topLevel = this;

   ListView::createListView(this);
   listview->initListView();
   setCentralWidget(listview->widget());
   resize(listview->widget()->sizeHint().width(), 400);

   createActions();
   createGUI();

   m_dcopIface = new KBookmarkEditorIface();

   connect(kapp->clipboard(), SIGNAL( dataChanged() ),      SLOT( slotClipboardDataChanged() ));
   connect(&m_commandHistory, SIGNAL( commandExecuted() ),  SLOT( slotCommandExecuted() ));
   connect(&m_commandHistory, SIGNAL( documentRestored() ), SLOT( slotDocumentRestored() ));

   listview->connectSignals();

   KGlobal::locale()->insertCatalogue("libkonq");

   construct();

   KEBListViewItem *item = listview->getItemAtAddress(address);
   if (!item) {
      item = listview->getFirstChild();
   }
   listview->setCurrent(item);
   item->setSelected(true);
}

void KEBTopLevel::construct() {
   MyManager::self()->createManager(this, m_bookmarksFilename);

   listview->updateListViewSetup(m_readOnly);
   listview->fillWithGroup(BkManagerAccessor::mgr()->root());

   slotClipboardDataChanged();

   resetActions();
   updateActions();

   setAutoSaveSettings();
   setModifiedFlag(false);
   cmdHistory->docSaved();
}

KEBTopLevel::~KEBTopLevel() {
   s_topLevel = 0;
   delete m_dcopIface;
}

/* ------------------------------------------------------------- */
//                             GUICORE
/* ------------------------------------------------------------- */

#define actn ActionsImpl::self()

void KEBTopLevel::createActions() {
   (void) KStdAction::open(this, SLOT( slotLoad() ), actionCollection());
   (void) KStdAction::save(this, SLOT( slotSave() ), actionCollection());
   (void) KStdAction::saveAs(this, SLOT( slotSaveAs() ), actionCollection());
   (void) KStdAction::quit(this, SLOT( close() ), actionCollection());
   (void) KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings() ), actionCollection());
   (void) KStdAction::configureToolbars(this, SLOT( slotConfigureToolbars() ), actionCollection());

   (void) KStdAction::cut(actn, SLOT( slotCut() ), actionCollection());
   (void) KStdAction::copy(actn, SLOT( slotCopy() ), actionCollection());
   (void) KStdAction::paste(actn, SLOT( slotPaste() ), actionCollection());

   (void) new KToggleAction(
                      i18n("&Auto-Save on Program Close"), 0, 
                      this, SLOT( slotSaveOnClose() ), actionCollection(), "settings_saveonclose");

   (void) new KToggleAction(
                      i18n("&Show Netscape Bookmarks in Konqueror Windows"), 0, 
                      actn, SLOT( slotShowNS() ), actionCollection(), "settings_showNS");
   (void) new KAction(i18n("&Delete"), "editdelete", Key_Delete, 
                      actn, SLOT( slotDelete() ), actionCollection(), "delete");
   (void) new KAction(i18n("Rename"), "text", Key_F2, 
                      actn, SLOT( slotRename() ), actionCollection(), "rename");
   (void) new KAction(i18n("C&hange URL"), "text", Key_F3, 
                      actn, SLOT( slotChangeURL() ), actionCollection(), "changeurl");
   (void) new KAction(i18n("Chan&ge Icon..."), 0, 
                      actn, SLOT( slotChangeIcon() ), actionCollection(), "changeicon");
   (void) new KAction(i18n("Update Favicon"), 0, 
                      actn, SLOT( slotUpdateFavIcon() ), actionCollection(), "updatefavicon");
   (void) new KAction(i18n("&Create New Folder..."), "folder_new", CTRL+Key_N, 
                      actn, SLOT( slotNewFolder() ), actionCollection(), "newfolder");
   (void) new KAction(i18n("Create &New Bookmark"), "www", 0, 
                      actn, SLOT( slotNewBookmark() ), actionCollection(), "newbookmark");
   (void) new KAction(i18n("&Insert Separator"), CTRL+Key_I, 
                      actn, SLOT( slotInsertSeparator() ), actionCollection(), "insertseparator");
   (void) new KAction(i18n("&Sort Alphabetically"), 0, 
                      actn, SLOT( slotSort() ), actionCollection(), "sort");
   (void) new KAction(i18n("Set as T&oolbar Folder"), "bookmark_toolbar", 0, 
                      actn, SLOT( slotSetAsToolbar() ), actionCollection(), "setastoolbar");
   (void) new KAction(i18n("&Expand All Folders"), 0, 
                      actn, SLOT( slotExpandAll() ), actionCollection(), "expandall");
   (void) new KAction(i18n("Collapse &All Folders"), 0, 
                      actn, SLOT( slotCollapseAll() ), actionCollection(), "collapseall" );
   (void) new KAction(i18n("&Open in Konqueror"), "fileopen", 0, 
                      actn, SLOT( slotOpenLink() ), actionCollection(), "openlink" );
   (void) new KAction(i18n("Check &Status"), "bookmark", 0, 
                      actn, SLOT( slotTestSelection() ), actionCollection(), "testlink" );
   (void) new KAction(i18n("&Find..."), 0, 
                      actn, SLOT( slotSearch() ), actionCollection(), "search" ); // "find" ?
   (void) new KAction(i18n("Check Status: &All"), 0, 
                      actn, SLOT( slotTestAll() ), actionCollection(), "testall" );
   (void) new KAction(i18n("Update All &Favicons"), 0, 
                      actn, SLOT( slotUpdateAllFavIcons() ), actionCollection(), "updateallfavicons" );
   (void) new KAction(i18n("Cancel &Checks"), 0, 
                      actn, SLOT( slotCancelAllTests() ), actionCollection(), "canceltests" );
   (void) new KAction(i18n("Cancel &Favicon Updates"), 0, 
                      actn, SLOT( slotCancelFavIconUpdates() ), actionCollection(), "cancelfaviconupdates" );
   (void) new KAction(i18n("Cancel &Search"), 0, 
                      actn, SLOT( slotCancelSearch() ), actionCollection(), "cancelsearch" );
   (void) new KAction(i18n("Import &Netscape Bookmarks..."), "netscape", 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importNS");
   (void) new KAction(i18n("Import &Opera Bookmarks..."), "opera", 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importOpera");
   (void) new KAction(i18n("Import &Galeon Bookmarks..."), 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importGaleon");
   (void) new KAction(i18n("Import &KDE2 Bookmarks..."), 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importKDE2");
   (void) new KAction(i18n("&Import IE Bookmarks..."), 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importIE");
   (void) new KAction(i18n("Import &Mozilla Bookmarks..."), "mozilla", 0, 
                      actn, SLOT( slotImport() ), actionCollection(), "importMoz");
   (void) new KAction(i18n("&Export to Netscape Bookmarks"), "netscape", 0, 
                      actn, SLOT( slotExportNS() ), actionCollection(), "exportNS");
   (void) new KAction(i18n("Export to &Mozilla Bookmarks..."), "mozilla", 0, 
                      actn, SLOT( slotExportMoz() ), actionCollection(), "exportMoz");
}

void KEBTopLevel::resetActions() {
   // DESIGN - try to remove usage of this
   disableDynamicActions(actionCollection()->actions());

   stateChanged("normal");

   if (!m_readOnly) {
      stateChanged("notreadonly");
   }

   static_cast<KToggleAction*>(actionCollection()->action("settings_saveonclose"))
      ->setChecked(m_saveOnClose);

   static_cast<KToggleAction*>(actionCollection()->action("settings_showNS"))
      ->setChecked(MyManager::self()->showNSBookmarks());
}

void KEBTopLevel::slotSaveOnClose() {
   m_saveOnClose 
      = static_cast<KToggleAction*>(actionCollection()->action("settings_saveonclose"))->isChecked();
}

bool KEBTopLevel::nsShown() {
   return static_cast<KToggleAction*>(actionCollection()->action("settings_showNS"))->isChecked();
}

void KEBTopLevel::updateActions() {
   listview->updateLastAddress();
   setActionsEnabled(listview->getSelectionAbilities());
}

void KEBTopLevel::setActionsEnabled(SelcAbilities sa) {
   KActionCollection * coll = actionCollection();

#define ea(a,b) coll->action(a)->setEnabled(b)

   bool t2 = !m_readOnly && sa.itemSelected;
   bool t4 = !m_readOnly && sa.singleSelect && !sa.root && !sa.separator;
   bool t5 = !m_readOnly && !sa.multiSelect;

   ea("edit_copy",         sa.itemSelected);
   ea("delete",            t2 && !sa.root);
   ea("edit_cut",          t2 && !sa.root);
   ea("edit_paste",        t2 && m_canPaste);

   ea("rename",            t4);
   ea("changeicon",        t4);
   ea("changeurl",         t4 && !sa.group);

   ea("newfolder",         t5);
   ea("newbookmark",       t5);
   ea("insertseparator",   t5);

   ea("expandall",         true);
   ea("collapseall",       true);
   ea("openlink",          sa.itemSelected && !sa.urlIsEmpty && !sa.group && !sa.separator);

   ea("search",            !sa.multiSelect);

   ea("testall",           !m_readOnly && sa.notEmpty);
   ea("testlink",          t2 && !sa.separator);

   ea("updateallfavicons", !m_readOnly && sa.notEmpty);
   ea("updatefavicon",     t2 && !sa.separator);

   ea("sort",              t5 && sa.group);
   ea("setastoolbar",      t5 && sa.group);
}

// DESIGN clean up this sh*t

void KEBTopLevel::setCancelSearchEnabled(bool enabled) {
   actionCollection()->action("cancelsearch")->setEnabled(enabled);
}

void KEBTopLevel::setCancelFavIconUpdatesEnabled(bool enabled) {
   actionCollection()->action("cancelfaviconupdates")->setEnabled(enabled);
}

void KEBTopLevel::setCancelTestsEnabled(bool enabled) {
   actionCollection()->action("canceltests")->setEnabled(enabled);
}

void KEBTopLevel::setModifiedFlag(bool modified) {
   QString caption = i18n("Bookmark Editor");
   m_modified = modified;

#if 0
   if (filename != default filename) {
      caption += QString(" [%2]").arg(filename.name());
   }
#endif

   if (m_readOnly) {
      m_modified = false;
      caption += QString(" [%2]").arg(i18n("Read Only"));
   }

   setCaption(caption, m_modified);

   // AK - commented due to usability bug by zander 
   // AK - on second thoughts. this is just wrong. and against
   // the style guide. maybe zander just saw a bug. doubt it though...
   // actionCollection()->action("file_save")->setEnabled(m_modified);

   // only update when non-modified
   // - this means that when we have modifications
   //   changes are sent via dcop rather than via
   //   a reload - which would loose user changes
   MyManager::self()->setUpdate(!m_modified); 
}

void KEBTopLevel::slotClipboardDataChanged() {
   kdDebug() << "KEBTopLevel::slotClipboardDataChanged" << endl;
   if (!m_readOnly) {
      m_canPaste = KBookmarkDrag::canDecode(KEBClipboard::get());
      ListView::self()->emitSlotSelectionChanged();
   }
}

/* ------------------------------------------------------------- */
//                      COMMAND HISTORY ACCESSORS
/* ------------------------------------------------------------- */

void KEBTopLevel::didCommand(KCommand *cmd) {
   if (cmd) {
      m_commandHistory.addCommand(cmd, false);
      emit slotCommandExecuted();
   }
}

void KEBTopLevel::addCommand(KCommand *cmd) {
   if (cmd) {
      m_commandHistory.addCommand(cmd);
   }
}

void KEBTopLevel::docSaved() {
   m_commandHistory.documentSaved();
}

void KEBTopLevel::clearHistory() {
   m_commandHistory.clear();
}

void KEBTopLevel::emitSlotCommandExecuted() {
   emit slotCommandExecuted();
}

/* ------------------------------------------------------------- */

// DESIGN - poinless drivel

void KEBTopLevel::setAllOpen(bool open) {
   listview->setOpen(open);
   setModifiedFlag(true);
}

/* ------------------------------------------------------------- */
//                        GUI_CALLBACK
/* ------------------------------------------------------------- */

// LATER - move
void KEBTopLevel::slotCommandExecuted() {
   if (!m_readOnly) {
      kdDebug() << "KEBTopLevel::slotCommandExecuted" << endl;
      setModifiedFlag(true);
      listview->updateListView();
      listview->emitSlotSelectionChanged();
      updateActions();
   }
}

void KEBTopLevel::slotDocumentRestored() {
   if (m_readOnly) {
      return;
   }
   // called when undoing the very first action - or the first one after
   // saving. the "document" is set to "non modified" in that case.
   setModifiedFlag(false);
}

void KEBTopLevel::slotBookmarksChanged(const QString &, const QString &caller) {
   // this is called when someone changes bookmarks in konqueror....
   kdDebug() << "FIXME: " << caller << " == " << kapp->name() << "?" << endl;
   // TODO umm.. what happens if a readonly gets a update for a non-readonly???
   // the non-readonly maybe has a pretty much random kapp->name() ??? umm...
   if ((caller != kapp->name()) && !m_modified) {
      kdDebug() << "KEBTopLevel::slotBookmarksChanged" << endl;
      // DESIGN - is this logic really unique?
      clearHistory();
      listview->fillWithGroup(BkManagerAccessor::mgr()->root());
      updateActions();
   }
}

/* ------------------------------------------------------------- */
// GUI - STANDARD
/* ------------------------------------------------------------- */

void KEBTopLevel::slotConfigureKeyBindings() {
   KKeyDialog::configure(actionCollection());
}

void KEBTopLevel::slotConfigureToolbars() {
   saveMainWindowSettings(KGlobal::config(), "MainWindow");
   KEditToolbar dlg(actionCollection());
   connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
   dlg.exec();
}

void KEBTopLevel::slotNewToolbarConfig() {
   // called when OK or Apply is clicked
   createGUI();
   applyMainWindowSettings(KGlobal::config(), "MainWindow");
}

/* ------------------------------------------------------------- */
// DOCUMENT
/* ------------------------------------------------------------- */

bool KEBTopLevel::save() {
   if (!MyManager::self()->managerSave()) {
      return false;
   }
   MyManager::self()->notifyManagers();
   setModifiedFlag(false);
   cmdHistory->docSaved(); // PRIVATE
   return true;
}

bool KEBTopLevel::queryClose() {
   if (!m_modified) {
      return true;
   }

   if (m_saveOnClose) {
      return save();
   }

   switch (
      KMessageBox::warningYesNoCancel(
         this, i18n("The bookmarks have been modified.\nSave changes?")) 
   ) {
      case KMessageBox::Yes:
         return save();
      case KMessageBox::No:
         return true;
      default: // case KMessageBox::Cancel:
         return false;
   }
}

void KEBTopLevel::slotLoad() {
   if (!queryClose()) {
      return;
   }
   // TODO - add a few default place to the file dialog somehow?,
   //      - e.g kfile bookmarks +  normal bookmarks file dir
   QString bookmarksFile = KFileDialog::getOpenFileName(QString::null, "*.xml", this);
   if (!bookmarksFile.isNull()) {
      m_bookmarksFilename = bookmarksFile;
      construct();
   }
}

void KEBTopLevel::slotSave() {
   (void)save();
}

void KEBTopLevel::slotSaveAs() {
   QString saveFilename = KFileDialog::getSaveFileName(QString::null, "*.xml", this);
   if(!saveFilename.isEmpty()) {
      MyManager::self()->saveAs(saveFilename);
   }
}

#include "toplevel.moc"
