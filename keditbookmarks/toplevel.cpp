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
#include <qsplitter.h>

#include <klocale.h>
#include <kdebug.h>

#include <kapplication.h>
#include <kstdaction.h>
#include <kaction.h>
#include <dcopclient.h>
#include <dcopref.h>

#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kfiledialog.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kbookmarkexporter.h>

#include "listview.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "search.h"

#include "toplevel.h"

#include "bookmarkinfo.h"

CmdHistory* CmdHistory::s_self = 0;

CmdHistory::CmdHistory(KActionCollection *collection) : m_commandHistory(collection) {
   connect(&m_commandHistory, SIGNAL( commandExecuted() ),  
                              SLOT( slotCommandExecuted() ));
   connect(&m_commandHistory, SIGNAL( documentRestored() ), 
                              SLOT( slotDocumentRestored() ));
   assert(!s_self);
   s_self = this;
}

CmdHistory* CmdHistory::self() {
   assert(s_self); 
   return s_self;
}

void CmdHistory::slotCommandExecuted() {
   KEBApp::self()->notifyCommandExecuted();
}

void CmdHistory::slotDocumentRestored() {
   // called when undoing the very first action - or the first one after
   // saving. the "document" is set to "non modified" in that case.
   if (!KEBApp::self()->readonly()) {
      KEBApp::self()->setModifiedFlag(false);
   }
}

void CmdHistory::notifyDocSaved() { 
   m_commandHistory.documentSaved();
}

void CmdHistory::didCommand(KCommand *cmd) {
   if (cmd) {
      m_commandHistory.addCommand(cmd, false);
      KEBApp::self()->notifyCommandExecuted();
   }
}

void CmdHistory::addCommand(KCommand *cmd) {
   if (cmd) {
      m_commandHistory.addCommand(cmd);
   }
}

void CmdHistory::clearHistory() {
   m_commandHistory.clear();
}

/* -------------------------- */

CurrentMgr *CurrentMgr::s_mgr = 0;

KBookmark CurrentMgr::bookmarkAt(const QString & a) { 
   return self()->mgr()->findByAddress(a); 
}

bool CurrentMgr::managerSave() { return mgr()->save(); }
void CurrentMgr::saveAs(const QString &fileName) { mgr()->saveAs(fileName); }
void CurrentMgr::setUpdate(bool update) { mgr()->setUpdate(update); }
QString CurrentMgr::path() { return mgr()->path(); }
bool CurrentMgr::showNSBookmarks() { return mgr()->showNSBookmarks(); }

void CurrentMgr::createManager(const QString &filename) {
   if (m_mgr) {
      disconnect(m_mgr, 0, 0, 0);
      // still todo - delete old m_mgr
   }

   m_mgr = KBookmarkManager::managerForFile(filename, false);

   connect(m_mgr, SIGNAL( changed(const QString &, const QString &) ),
                  SLOT( slotBookmarksChanged(const QString &, const QString &) ));
}

void CurrentMgr::slotBookmarksChanged(const QString &, const QString &caller) {
   // kdDebug() << "CurrentMgr::slotBookmarksChanged" << endl;
   if ((caller.latin1() != kapp->dcopClient()->appId()) && !KEBApp::self()->modified()) {
      // TODO 
      // umm.. what happens if a readonly gets a update for a non-readonly???
      // the non-readonly maybe has a pretty much random kapp->name() ??? umm...
      CmdHistory::self()->clearHistory();
      ListView::self()->updateListView();
      KEBApp::self()->updateActions();
   }
}

void CurrentMgr::notifyManagers() {
   QCString objId("KBookmarkManager-");
   objId += mgr()->path().utf8();
   DCOPRef("*", objId).send("notifyCompleteChange", QString::fromLatin1(kapp->dcopClient()->appId()));
}

void CurrentMgr::doExport(bool moz) {
   QString path = 
          (moz)
        ? KNSBookmarkImporter::mozillaBookmarksFile(true)
        : KNSBookmarkImporter::netscapeBookmarksFile(true);
   if (!path.isEmpty()) {
      KNSBookmarkExporter exporter(mgr(), path);
      exporter.write(moz);
   }
}

QString CurrentMgr::correctAddress(const QString &address) {
   return mgr()->findByAddress(address, true).address();
}

/* -------------------------- */

KEBApp *KEBApp::s_topLevel = 0;

KEBApp::KEBApp(const QString & bookmarksFile, bool readonly, const QString &address)
   : KMainWindow(), m_dcopIface(0) {

   m_bookmarksFilename = bookmarksFile;
   m_readOnly = readonly;
   m_cmdHistory = new CmdHistory(actionCollection());

   s_topLevel = this;

   QSplitter *vsplitter = new QSplitter(this);
   m_iSearchLineEdit = new KLineEdit(vsplitter);
   m_iSearchLineEdit->setMinimumHeight(20);
   m_iSearchLineEdit->setMaximumHeight(20);

   QSplitter *splitter = new QSplitter(vsplitter);
   ListView::createListViews(splitter);
   ListView::self()->initListViews();
   ListView::self()->setInitialAddress(address);

   BookmarkInfoWidget *bkinfo = new BookmarkInfoWidget(vsplitter);
   vsplitter->setOrientation(QSplitter::Vertical);
   vsplitter->setSizes(QValueList<int>() << bkinfo->sizeHint().height() << 380 << bkinfo->sizeHint().height() );

   setCentralWidget(vsplitter);
   resize(ListView::self()->widget()->sizeHint().width()
         + 0 /* TODO - other split view */, 500);

   createActions();
   createGUI();

   m_dcopIface = new KBookmarkEditorIface();

   connect(kapp->clipboard(), SIGNAL( dataChanged() ),      SLOT( slotClipboardDataChanged() ));

   ListView::self()->connectSignals();

   KGlobal::locale()->insertCatalogue("libkonq");

   construct();

   updateActions();
}

void KEBApp::construct() {
   CurrentMgr::self()->createManager(m_bookmarksFilename);

   ListView::self()->updateListViewSetup(m_readOnly);
   ListView::self()->updateListView();

   connect(m_iSearchLineEdit, SIGNAL( textChanged(const QString &) ),
           Searcher::self(),  SLOT( slotSearchTextChanged(const QString &) ));

   slotClipboardDataChanged();

   readConfig();
   resetActions();
   updateActions();

   setAutoSaveSettings();
   setModifiedFlag(false);
   m_cmdHistory->notifyDocSaved();
}

KEBApp::~KEBApp() {
   s_topLevel = 0;
   delete m_dcopIface;
}

void KEBApp::createActions() {

   ActionsImpl *actn = ActionsImpl::self();

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
                      i18n("Advanced Add Bookmark in Konqueror"), 0,
                      this, SLOT( slotAdvancedAddBookmark() ), actionCollection(), 
                      "settings_advancedaddbookmark");
   (void) new KToggleAction(
                      i18n("&Show Netscape Bookmarks in Konqueror Windows"), 0,
                      actn, SLOT( slotShowNS() ), actionCollection(), "settings_showNS");
   (void) new KAction(i18n("&Delete"), "editdelete", Key_Delete,
                      actn, SLOT( slotDelete() ), actionCollection(), "delete");
   (void) new KAction(i18n("Rename"), "text", Key_F2,
                      actn, SLOT( slotRename() ), actionCollection(), "rename");
   (void) new KAction(i18n("C&hange URL"), "text", Key_F3,
                      actn, SLOT( slotChangeURL() ), actionCollection(), "changeurl");
   (void) new KAction(i18n("C&hange Comment"), "text", Key_F4,
                      actn, SLOT( slotChangeComment() ), actionCollection(), "changecomment");
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

   (void) new KAction(i18n("Check Status: &All"), 0,
                      actn, SLOT( slotTestAll() ), actionCollection(), "testall" );
   (void) new KAction(i18n("Update All &Favicons"), 0,
                      actn, SLOT( slotUpdateAllFavIcons() ), actionCollection(), "updateallfavicons" );
   (void) new KAction(i18n("Cancel &Checks"), 0,
                      actn, SLOT( slotCancelAllTests() ), actionCollection(), "canceltests" );
   (void) new KAction(i18n("Cancel &Favicon Updates"), 0,
                      actn, SLOT( slotCancelFavIconUpdates() ), actionCollection(), "cancelfaviconupdates" );
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

KToggleAction* KEBApp::getToggleAction(const char *action) {
   return static_cast<KToggleAction*>(actionCollection()->action(action));
}

void KEBApp::resetActions() {
   stateChanged("disablestuff");
   stateChanged("normal");

   if (!m_readOnly) {
      stateChanged("notreadonly");
   }

   getToggleAction("settings_saveonclose")->setChecked(m_saveOnClose);
   getToggleAction("settings_advancedaddbookmark")->setChecked(m_advancedAddBookmark);
   getToggleAction("settings_showNS")->setChecked(CurrentMgr::self()->showNSBookmarks());
}

void KEBApp::readConfig() {
   KConfig config("kbookmarkrc", false, false);
   config.setGroup("Bookmarks");
   m_advancedAddBookmark = config.readBoolEntry("AdvancedAddBookmark", false);

   KConfig appconfig("keditbookmarksrc", false, false);
   appconfig.setGroup("General");
   m_saveOnClose = appconfig.readBoolEntry("Save On Close", false);
}

void KEBApp::slotAdvancedAddBookmark() {
   m_advancedAddBookmark = getToggleAction("settings_advancedaddbookmark")->isChecked();
   KConfig config("kbookmarkrc", false, false);
   config.setGroup("Bookmarks");
   config.writeEntry("AdvancedAddBookmark", m_advancedAddBookmark);
}

void KEBApp::slotSaveOnClose() {
   m_saveOnClose = getToggleAction("settings_saveonclose")->isChecked();
   KConfig appconfig("keditbookmarksrc", false, false);
   appconfig.setGroup("General");
   appconfig.writeEntry("Save On Close", m_saveOnClose);
}

bool KEBApp::nsShown() {
   return getToggleAction("settings_showNS")->isChecked();
}

// this should be pushed from listview, not pulled
void KEBApp::updateActions() {
   setActionsEnabled(ListView::self()->getSelectionAbilities());
}

void KEBApp::setActionsEnabled(SelcAbilities sa) {
   KActionCollection * coll = actionCollection();

#define ENABLE(a) coll->action(a)->setEnabled(true);

   if (sa.itemSelected) {
      ENABLE("edit_copy");
      if (!sa.urlIsEmpty && !sa.group && !sa.separator) {
         ENABLE("openlink");
      }
   }

   if (!m_readOnly) {
      if (sa.notEmpty) {
         ENABLE("testall");
         ENABLE("updateallfavicons");
      }

      if (sa.itemSelected) {
         if (!sa.root) {
            ENABLE("delete");
            ENABLE("edit_cut");
         }
         if (m_canPaste) {
            ENABLE("edit_paste");
         }
         if (!sa.separator) {
            ENABLE("testlink");
            ENABLE("updatefavicon");
         }
      }

      if (sa.singleSelect && !sa.root && !sa.separator) {
         ENABLE("rename");
         ENABLE("changeicon");
         ENABLE("changecomment");
         if (!sa.group) {
            ENABLE("changeurl");
         }
      }

      if (!sa.multiSelect) {
         ENABLE("newfolder");
         ENABLE("newbookmark");
         ENABLE("insertseparator");
         if (sa.group) {
            ENABLE("sort");
            ENABLE("setastoolbar");
         }
      }
   }
}

void KEBApp::setCancelFavIconUpdatesEnabled(bool enabled) {
   actionCollection()->action("cancelfaviconupdates")->setEnabled(enabled);
}

void KEBApp::setCancelTestsEnabled(bool enabled) {
   actionCollection()->action("canceltests")->setEnabled(enabled);
}

void KEBApp::setModifiedFlag(bool modified) {
   m_modified = modified;

   QString caption = i18n("Bookmark Editor");

   if (m_bookmarksFilename != KBookmarkManager::userBookmarksManager()->path()) {
      caption += QString(" %1").arg(m_bookmarksFilename);
   }

   if (m_readOnly) {
      m_modified = false;
      caption += QString(" [%1]").arg(i18n("Read Only"));
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
   CurrentMgr::self()->setUpdate(!m_modified);
}

void KEBApp::slotClipboardDataChanged() {
   // kdDebug() << "KEBApp::slotClipboardDataChanged" << endl;
   if (!m_readOnly) {
      m_canPaste = KBookmarkDrag::canDecode(kapp->clipboard()->data(QClipboard::Clipboard));
      ListView::self()->emitSlotSelectionChanged();
   }
}

/* -------------------------- */

void KEBApp::notifyCommandExecuted() {
   // kdDebug() << "KEBApp::notifyCommandExecuted()" << endl;
   if (!m_readOnly) {
      setModifiedFlag(true);
      ListView::self()->updateListView();
      ListView::self()->emitSlotSelectionChanged();
      updateActions();
   }
}

/* -------------------------- */

void KEBApp::slotConfigureKeyBindings() {
   KKeyDialog::configure(actionCollection());
}

void KEBApp::slotConfigureToolbars() {
   saveMainWindowSettings(KGlobal::config(), "MainWindow");
   KEditToolbar dlg(actionCollection());
   connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
   dlg.exec();
}

void KEBApp::slotNewToolbarConfig() {
   // called when OK or Apply is clicked
   createGUI();
   applyMainWindowSettings(KGlobal::config(), "MainWindow");
}

/* -------------------------- */

bool KEBApp::save() {
   if (!CurrentMgr::self()->managerSave()) {
      return false;
   }
   CurrentMgr::self()->notifyManagers();
   setModifiedFlag(false);
   m_cmdHistory->notifyDocSaved();
   return true;
}

bool KEBApp::queryClose() {
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

void KEBApp::slotLoad() {
   if (!queryClose()) {
      return;
   }
   QString bookmarksFile = KFileDialog::getOpenFileName(QString::null, "*.xml", this);
   if (!bookmarksFile.isNull()) {
      m_bookmarksFilename = bookmarksFile;
      construct();
   }
}

void KEBApp::slotSave() {
   (void)save();
}

void KEBApp::slotSaveAs() {
   QString saveFilename = KFileDialog::getSaveFileName(QString::null, "*.xml", this);
   if(!saveFilename.isEmpty()) {
      CurrentMgr::self()->saveAs(saveFilename);
   }
}

#include "toplevel.moc"
