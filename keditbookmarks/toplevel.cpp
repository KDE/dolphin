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
#include <qlayout.h>
#include <qlabel.h>

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
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkexporter.h>

#include "listview.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "search.h"

#include "toplevel.h"

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

KBookmark CurrentMgr::bookmarkAt(const QString &a) { 
   return self()->mgr()->findByAddress(a); 
}

bool CurrentMgr::managerSave() { return mgr()->save(); }
void CurrentMgr::saveAs(const QString &fileName) { mgr()->saveAs(fileName); }
void CurrentMgr::setUpdate(bool update) { mgr()->setUpdate(update); }
QString CurrentMgr::path() const { return mgr()->path(); }
bool CurrentMgr::showNSBookmarks() const { return mgr()->showNSBookmarks(); }

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

class HTMLExporter : private KBookmarkGroupTraverser {
public:
   HTMLExporter();
   void write(const KBookmarkGroup &, const QString &);
private:
   virtual void visit(const KBookmark &);
   virtual void visitEnter(const KBookmarkGroup &);
   virtual void visitLeave(const KBookmarkGroup &);
private:
   QString m_string;
   QTextStream m_out;
   int m_level;
};

HTMLExporter::HTMLExporter() 
   : m_out(&m_string, IO_WriteOnly) {
   m_level = 0;
}

void HTMLExporter::write(const KBookmarkGroup &grp, const QString &filename) {
   HTMLExporter exporter;
   QFile file(filename);
   if (!file.open(IO_WriteOnly)) {
      kdError(7043) << "Can't write to file " << filename << endl;
      return;
   }
   QTextStream fstream(&file);
   fstream.setEncoding(QTextStream::UnicodeUTF8);
   traverse(grp);
   fstream << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" << endl;
   fstream << "<HTML><HEAD><TITLE>My Bookmarks</TITLE></HEAD>" << endl;
   fstream << "<BODY>" << endl;
   fstream << m_string;
   fstream << "</BODY></HTML>" << endl;
}

void HTMLExporter::visit(const KBookmark &bk) {
   // kdDebug() << "visit(" << bk.text() << ")" << endl;
   m_out << "<A href=\"" << bk.url().url().utf8() << "\">";
   m_out << bk.fullText() << "</A><BR>" << endl;
}

void HTMLExporter::visitEnter(const KBookmarkGroup &grp) {
   // kdDebug() << "visitEnter(" << grp.text() << ")" << endl;
   m_out << "<H3>" << grp.fullText() << "</H3>" << endl;
   m_out << "<P style=\"margin-left: " 
         << (m_level * 3) << "em\">" << endl;
   m_level++;
} 

void HTMLExporter::visitLeave(const KBookmarkGroup &) {
   // kdDebug() << "visitLeave()" << endl;
   m_out << "</P>" << endl;
   m_level--;
   if (m_level != 0)
      m_out << "<P style=\"left-margin: " 
            << (m_level * 3) << "em\">" << endl;
}

void CurrentMgr::doExport(ExportType type) {
   // TODO - add a factory and make all this use the base class
   if (type == OperaExport) {
      QString path = KOperaBookmarkImporterImpl().findDefaultLocation(true);
      KOperaBookmarkExporterImpl exporter(mgr(), path);
      exporter.write(mgr()->root());
      return;
   } else if (type == HTMLExport) {
      QString path = KFileDialog::getSaveFileName( QDir::homeDirPath(),
                                                   i18n("*.html|HTML Bookmark Listing") );
      HTMLExporter exporter;
      exporter.write(mgr()->root(), path);
      return;
   } else if (type == IEExport) {
      QString path = KIEBookmarkImporterImpl().findDefaultLocation(true);
      KIEBookmarkExporterImpl exporter(mgr(), path);
      exporter.write(mgr()->root());
      return;
   }
   bool moz = (type == MozillaExport);
   QString path = (moz)
                ? KNSBookmarkImporter::mozillaBookmarksFile(true)
                : KNSBookmarkImporter::netscapeBookmarksFile(true);
   if (!path.isEmpty()) {
      KNSBookmarkExporter exporter(mgr(), path);
      exporter.write(moz);
   }
}

QString CurrentMgr::correctAddress(const QString &address) const {
   return mgr()->findByAddress(address, true).address();
}

/* -------------------------- */

KEBApp *KEBApp::s_topLevel = 0;

void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
   m_title_le->setText(bk.text());
   m_url_le->setText(bk.url().url());
   m_comment_le->setText("a comment");
   m_moddate_le->setText("the modification date");
   m_credate_le->setText("the creation date");
}

BookmarkInfoWidget::BookmarkInfoWidget(
   QWidget * parent, const char * name
) : QWidget (parent, name) {
   QBoxLayout *vbox = new QVBoxLayout(this);
   QGridLayout *grid = new QGridLayout(vbox, 3, 4);

   m_title_le = new KLineEdit(this);
   m_title_le->setReadOnly(true);
   grid->addWidget(m_title_le, 0, 1);
   grid->addWidget(
            new QLabel(m_title_le, i18n("Name:"), this), 
            0, 0);

   m_url_le = new KLineEdit(this);
   m_url_le->setReadOnly(true);
   grid->addWidget(m_url_le, 1, 1);
   grid->addWidget(
            new QLabel(m_url_le, i18n("Location:"), this), 
            1, 0);

   m_comment_le = new KLineEdit(this);
   m_comment_le->setReadOnly(true);
   grid->addWidget(m_comment_le, 2, 1);
   grid->addWidget(
            new QLabel(m_comment_le, i18n("Comment:"), this), 
            2, 0);

   m_moddate_le = new KLineEdit(this);
   m_moddate_le->setReadOnly(true);
   grid->addWidget(m_moddate_le, 0, 3);
   grid->addWidget(
            new QLabel(m_moddate_le, i18n("First viewed:"), this), 
            0, 2 );

   m_credate_le = new KLineEdit(this);
   m_credate_le->setReadOnly(true);
   grid->addWidget(m_credate_le, 1, 3);
   grid->addWidget(
            new QLabel(m_credate_le, i18n("Viewed last:"), this), 
            1, 2);
}

class MagicKLineEdit : public KLineEdit {
public:
   MagicKLineEdit(const QString &text, QWidget *parent, const char *name = 0);
   virtual void focusOutEvent(QFocusEvent *ev);
   virtual void mousePressEvent(QMouseEvent *ev);
   virtual void focusInEvent(QFocusEvent *ev);
   void setGrayedText(const QString &text) { m_grayedText = text; }
   QString grayedText() const { return m_grayedText; }
private:
   QString m_grayedText;
};

MagicKLineEdit::MagicKLineEdit(
   const QString &text, QWidget *parent, const char *name
) : KLineEdit(text, parent, name), m_grayedText(text) {
   setPaletteForegroundColor(gray);
}

void MagicKLineEdit::focusInEvent(QFocusEvent *ev) {
   if (text() == m_grayedText)
      setText(QString::null);
   QLineEdit::focusInEvent(ev);
}

void MagicKLineEdit::focusOutEvent(QFocusEvent *ev) {
   if (text().isEmpty()) {
      setText(m_grayedText);
      setPaletteForegroundColor(gray); 
   }
   QLineEdit::focusOutEvent(ev);
}

void MagicKLineEdit::mousePressEvent(QMouseEvent *ev) {
   setPaletteForegroundColor(parentWidget()->paletteForegroundColor()); 
   QLineEdit::mousePressEvent(ev);
}

KEBApp::KEBApp(
   const QString &bookmarksFile, bool readonly, 
   const QString &address, bool browser, const QString &caption 
) : KMainWindow(), m_dcopIface(0), m_bookmarksFilename(bookmarksFile),
    m_caption(caption), m_readOnly(readonly), m_browser(browser) {

   m_cmdHistory = new CmdHistory(actionCollection());

   s_topLevel = this;

   int h = 20;

   QSplitter *vsplitter = new QSplitter(this);
   m_iSearchLineEdit = new MagicKLineEdit(i18n("Type here to search..."), vsplitter);
   m_iSearchLineEdit->setMinimumHeight(h);
   m_iSearchLineEdit->setMaximumHeight(h);

   readConfig();

   QSplitter *splitter = new QSplitter(vsplitter);
   ListView::createListViews(splitter);
   ListView::self()->initListViews();
   ListView::self()->setInitialAddress(address);

   m_bkinfo = new BookmarkInfoWidget(vsplitter);

   vsplitter->setOrientation(QSplitter::Vertical);
   vsplitter->setSizes(QValueList<int>() << h << 380 << m_bkinfo->sizeHint().height() );

   setCentralWidget(vsplitter);
   resize(ListView::self()->widget()->sizeHint().width(), vsplitter->sizeHint().height());

   createActions();
   if (m_browser)
      createGUI();
   else
      createGUI("keditbookmarks-genui.rc");

   m_dcopIface = new KBookmarkEditorIface();

   connect(kapp->clipboard(), SIGNAL( dataChanged() ),      SLOT( slotClipboardDataChanged() ));

   connect(m_iSearchLineEdit, SIGNAL( textChanged(const QString &) ),
           Searcher::self(),  SLOT( slotSearchTextChanged(const QString &) ));

   connect(m_iSearchLineEdit, SIGNAL( returnPressed() ),
           Searcher::self(),  SLOT( slotSearchNext() ));

   ListView::self()->connectSignals();

   KGlobal::locale()->insertCatalogue("libkonq");

   construct();

   updateActions();
}

void KEBApp::construct() {
   CurrentMgr::self()->createManager(m_bookmarksFilename);

   ListView::self()->updateListViewSetup(m_readOnly);
   ListView::self()->updateListView();
   ListView::self()->widget()->setFocus();

   slotClipboardDataChanged();

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

   if (m_browser) {
      (void) KStdAction::open(this, SLOT( slotLoad() ), actionCollection());
      (void) KStdAction::saveAs(this, SLOT( slotSaveAs() ), actionCollection());
   }

   (void) KStdAction::save(this, SLOT( slotSave() ), actionCollection());
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
   // TODO - these options should be a in a kcontrol thing
   (void) new KToggleAction(
                      i18n("Display Only Marked Bookmarks in Konqueror Bookmark Toolbar"), 0,
                      this, SLOT( slotFilteredToolbar() ), actionCollection(), 
                      "settings_filteredtoolbar");
   (void) new KToggleAction(
                      i18n("&Split View"), 0,
                      this, SLOT( slotSplitView() ), actionCollection(), "settings_splitview");
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
   (void) new KAction(i18n("Recursive Sort"), 0,
                      actn, SLOT( slotRecursiveSort() ), actionCollection(), "recursivesort");
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
   (void) new KAction(i18n("Show in T&oolbar"), "bookmark_toolbar", 0,
                      actn, SLOT( slotShowInToolbar() ), actionCollection(), "showintoolbar");
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
   (void) new KAction(i18n("Import All &Crash Sessions as Bookmarks..."), "opera", 0,
                      actn, SLOT( slotImport() ), actionCollection(), "importCrashes");
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
   (void) new KAction(i18n("&Export to Opera Bookmarks"), "opera", 0,
                      actn, SLOT( slotExportOpera() ), actionCollection(), "exportOpera");
   (void) new KAction(i18n("&Export to HTML Bookmarks"), "html", 0,
                      actn, SLOT( slotExportHTML() ), actionCollection(), "exportHTML");
   (void) new KAction(i18n("&Export to IE Bookmarks"), "ie", 0,
                      actn, SLOT( slotExportIE() ), actionCollection(), "exportIE");
   (void) new KAction(i18n("Export to &Mozilla Bookmarks..."), "mozilla", 0,
                      actn, SLOT( slotExportMoz() ), actionCollection(), "exportMoz");
}

KToggleAction* KEBApp::getToggleAction(const char *action) const {
   return static_cast<KToggleAction*>(actionCollection()->action(action));
}

void KEBApp::resetActions() {
   stateChanged("disablestuff");
   stateChanged("normal");

   if (!m_readOnly)
      stateChanged("notreadonly");

   getToggleAction("settings_saveonclose")->setChecked(m_saveOnClose);
   getToggleAction("settings_advancedaddbookmark")->setChecked(m_advancedAddBookmark);
   getToggleAction("settings_filteredtoolbar")->setChecked(m_filteredToolbar);
   getToggleAction("settings_splitview")->setChecked(m_splitView);
   getToggleAction("settings_showNS")->setChecked(CurrentMgr::self()->showNSBookmarks());
}

void KEBApp::readConfig() {
   if (m_browser) {
      KConfig config("kbookmarkrc", false, false);
      config.setGroup("Bookmarks");
      m_advancedAddBookmark = config.readBoolEntry("AdvancedAddBookmark", false);
      m_filteredToolbar = config.readBoolEntry("FilteredToolbar", false);
   }

   KConfig appconfig("keditbookmarksrc", false, false);
   appconfig.setGroup("General");
   m_saveOnClose = appconfig.readBoolEntry("Save On Close", false);
   m_splitView = appconfig.readBoolEntry("Split View", false);
}

static void writeConfigBool(
   const QString &rcfile, const QString &group, 
   const QString &entry, bool flag
) {
   KConfig config(rcfile, false, false);
   config.setGroup(group);
   config.writeEntry(entry, flag);
}

void KEBApp::slotAdvancedAddBookmark() {
   Q_ASSERT(m_browser);
   m_advancedAddBookmark = getToggleAction("settings_advancedaddbookmark")->isChecked();
   writeConfigBool("kbookmarkrc", "Bookmarks", "AdvancedAddBookmark", m_advancedAddBookmark);
}

void KEBApp::slotFilteredToolbar() {
   m_filteredToolbar = getToggleAction("settings_filteredtoolbar")->isChecked();
   writeConfigBool("kbookmarkrc", "Bookmarks", "FilteredToolbar", m_filteredToolbar);
}

void KEBApp::slotSplitView() {
   m_splitView = getToggleAction("settings_splitview")->isChecked();
   writeConfigBool("keditbookmarksrc", "General", "Split View", m_splitView);
}

void KEBApp::slotSaveOnClose() {
   m_saveOnClose = getToggleAction("settings_saveonclose")->isChecked();
   writeConfigBool("keditbookmarksrc", "General", "Save On Close", m_saveOnClose);
}

bool KEBApp::nsShown() const {
   return getToggleAction("settings_showNS")->isChecked();
}

// this should be pushed from listview, not pulled
void KEBApp::updateActions() {
   setActionsEnabled(ListView::self()->getSelectionAbilities());
}

void KEBApp::setActionsEnabled(SelcAbilities sa) {
   KActionCollection * coll = actionCollection();

   QStringList toEnable;

   if (sa.itemSelected) {
      toEnable << "edit_copy";
      if (!sa.urlIsEmpty && !sa.group && !sa.separator)
         toEnable << "openlink";
   }

   if (!m_readOnly) {
      if (sa.notEmpty)
         toEnable << "testall" << "updateallfavicons";

      if (sa.itemSelected) {
         if (!sa.root)
            toEnable << "delete" << "edit_cut";
         if (m_canPaste)
            toEnable << "edit_paste";
         if (!sa.separator)
            toEnable << "testlink" << "updatefavicon";
      }

      if (sa.singleSelect && !sa.root && !sa.separator) {
         toEnable << "rename" << "changeicon" << "changecomment";
         if (!sa.group)
            toEnable << "changeurl";
      }

      if (!sa.multiSelect) {
         toEnable << "newfolder" << "newbookmark" 
                  << "insertseparator" << "showintoolbar";
         if (sa.group)
            toEnable << "sort" << "recursivesort" << "setastoolbar";
      }
   }

   for ( QStringList::Iterator it = toEnable.begin(); 
         it != toEnable.end(); ++it )
      coll->action((*it).ascii())->setEnabled(true);
}

void KEBApp::setCancelFavIconUpdatesEnabled(bool enabled) {
   actionCollection()->action("cancelfaviconupdates")->setEnabled(enabled);
}

void KEBApp::setCancelTestsEnabled(bool enabled) {
   actionCollection()->action("canceltests")->setEnabled(enabled);
}

void KEBApp::setModifiedFlag(bool modified) {
   m_modified = modified && !m_readOnly;

   QString caption = m_caption.isNull() ? "" : (m_caption + " ");
   if (m_bookmarksFilename != KBookmarkManager::userBookmarksManager()->path())
      caption += (caption.isEmpty()?"":" - ") + m_bookmarksFilename;
   if (m_readOnly)
      caption += QString(" [%1]").arg(i18n("Read Only"));

   setCaption(caption, m_modified);

   // we receive dcop if modified 
   // rather than reparse notifies
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
   if (!CurrentMgr::self()->managerSave())
      return false;
   CurrentMgr::self()->notifyManagers();
   setModifiedFlag(false);
   m_cmdHistory->notifyDocSaved();
   return true;
}

bool KEBApp::queryClose() {
   if (!m_modified)
      return true;
   if (m_saveOnClose)
      return save();

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
   if (!queryClose())
      return;
   QString bookmarksFile = KFileDialog::getOpenFileName(QString::null, "*.xml", this);
   if (bookmarksFile.isNull())
      return;
   m_caption = QString::null;
   m_bookmarksFilename = bookmarksFile;
   construct();
}

void KEBApp::slotSave() {
   (void)save();
}

void KEBApp::slotSaveAs() {
   QString saveFilename = KFileDialog::getSaveFileName(QString::null, "*.xml", this);
   if (!saveFilename.isEmpty())
      CurrentMgr::self()->saveAs(saveFilename);
}

#include "toplevel.moc"
