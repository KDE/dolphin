// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et: 
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

#include "toplevel.h"

#include "bookmarkinfo.h"
#include "listview.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "exporters.h"

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

#include <kedittoolbar.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <klistviewsearchline.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

bool KEBApp::queryClose() {
    return ActionsImpl::self()->queryClose();
}

CmdHistory* CmdHistory::s_self = 0;

CmdHistory::CmdHistory(KActionCollection *collection) 
    : m_commandHistory(collection) {
    connect(&m_commandHistory, SIGNAL( commandExecuted() ),  
            SLOT( slotCommandExecuted() ));
    connect(&m_commandHistory, SIGNAL( documentRestored() ), 
            SLOT( slotDocumentRestored() ));
    assert(!s_self);
    s_self = this; // this is hacky
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
    if (!cmd)
        return;
    m_commandHistory.addCommand(cmd, false);
    CmdHistory::slotCommandExecuted();
}

void CmdHistory::addCommand(KCommand *cmd) {
    if (!cmd)
        return;
    m_commandHistory.addCommand(cmd);
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
    if ((caller.latin1() != kapp->dcopClient()->appId()) 
     && !KEBApp::self()->modified()) {
        // TODO 
        // umm.. what happens if a readonly gets a update 
        // for a non-readonly??? the non-readonly maybe 
        // has a pretty much random kapp->name() ???
        CmdHistory::self()->clearHistory();
        ListView::self()->updateListView();
        KEBApp::self()->updateActions();
    }
}

void CurrentMgr::updateStatus(QString url) {
    ListView::self()->updateStatus(url);
}

void CurrentMgr::notifyManagers() {
    KBookmarkGroup grp = mgr()->root();
    mgr()->emitChanged(grp);
}

void CurrentMgr::reloadConfig() {
    mgr()->emitConfigChanged();
}

QString CurrentMgr::correctAddress(const QString &address) const {
    return mgr()->findByAddress(address, true).address();
}

/* -------------------------- */

KEBApp *KEBApp::s_topLevel = 0;

KEBApp::KEBApp(
    const QString &bookmarksFile, bool readonly, 
    const QString &address, bool browser, const QString &caption 
) : KMainWindow(), m_dcopIface(0), m_bookmarksFilename(bookmarksFile),
    m_caption(caption), m_readOnly(readonly), m_browser(browser) {

    m_cmdHistory = new CmdHistory(actionCollection());

    s_topLevel = this;

    int h = 20;

    QSplitter *vsplitter = new QSplitter(this);

    KToolBar *quicksearch = new KToolBar(vsplitter, "search toolbar");
    
    KAction *resetQuickSearch = new KAction( i18n( "Reset Quick Search" ),
        QApplication::reverseLayout() ? "clear_left" : "locationbar_erase",
        0, actionCollection(), "reset_quicksearch" );
    resetQuickSearch->setWhatsThis( i18n( "<b>Reset Quick Search<b><br>"
        "Resets the quick search so that all bookmarks are shown again." ) );
    resetQuickSearch->plug( quicksearch );

    QLabel *lbl = new QLabel(i18n("Se&arch:"), quicksearch, "kde toolbar widget");
    
    KListViewSearchLine *searchLineEdit = new KListViewSearchLine(quicksearch, 0, "KListViewSearchLine");    
    quicksearch->setStretchableWidget(searchLineEdit);
    lbl->setBuddy(searchLineEdit);
    connect(resetQuickSearch, SIGNAL(activated()), searchLineEdit, SLOT(clear()));
    
    readConfig();

    QSplitter *splitter = new QSplitter(vsplitter);
    ListView::createListViews(splitter);
    ListView::self()->initListViews();
    ListView::self()->setInitialAddress(address);
    searchLineEdit->setListView(static_cast<KListView*>(ListView::self()->widget()));
    
    m_bkinfo = new BookmarkInfoWidget(vsplitter);

    vsplitter->setOrientation(QSplitter::Vertical);
    vsplitter->setSizes(QValueList<int>() << h << 380 
                                          << m_bkinfo->sizeHint().height() );

    setCentralWidget(vsplitter);
    resize(ListView::self()->widget()->sizeHint().width(), 
           vsplitter->sizeHint().height());

    createActions();
    if (m_browser)
        createGUI();
    else
        createGUI("keditbookmarks-genui.rc");

    m_dcopIface = new KBookmarkEditorIface();

    connect(kapp->clipboard(), SIGNAL( dataChanged() ),      
                               SLOT( slotClipboardDataChanged() ));

    ListView::self()->connectSignals();

    KGlobal::locale()->insertCatalogue("libkonq");

    m_canPaste = false;

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
    delete ActionsImpl::self();
    delete ListView::self();
}

KToggleAction* KEBApp::getToggleAction(const char *action) const {
    return static_cast<KToggleAction*>(actionCollection()->action(action));
}

void KEBApp::resetActions() {
    stateChanged("disablestuff");
    stateChanged("normal");

    if (!m_readOnly)
        stateChanged("notreadonly");

    getToggleAction("settings_saveonclose")
        ->setChecked(m_saveOnClose);
    // getToggleAction("settings_splitview")
    //      ->setChecked(m_splitView);
    getToggleAction("settings_showNS")
        ->setChecked(CurrentMgr::self()->showNSBookmarks());
}

void KEBApp::readConfig() {
    KConfig appconfig("keditbookmarksrc", false, false);
    appconfig.setGroup("General");
    m_saveOnClose = appconfig.readBoolEntry("Save On Close", false);
    m_splitView = false; // appconfig.readBoolEntry("Split View", false);
}

static void writeConfigBool(
    const QString &rcfile, const QString &group, 
    const QString &entry, bool flag
) {
    KConfig config(rcfile, false, false);
    config.setGroup(group);
    config.writeEntry(entry, flag);
    config.sync();
    CurrentMgr::self()->reloadConfig();
}

void KEBApp::slotSplitView() {
    Q_ASSERT( 0 );
#if 0
    m_splitView = getToggleAction("settings_splitview")->isChecked();
    writeConfigBool("keditbookmarksrc", "General", 
                    "Split View", m_splitView);
    sorryRelogin(this);
#endif
}

void KEBApp::slotSaveOnClose() {
    m_saveOnClose = getToggleAction("settings_saveonclose")->isChecked();
    writeConfigBool("keditbookmarksrc", "General", 
                    "Save On Close", m_saveOnClose);
}

bool KEBApp::nsShown() const {
    return getToggleAction("settings_showNS")->isChecked();
}

// this should be pushed from listview, not pulled
void KEBApp::updateActions() {
    resetActions();
    setActionsEnabled(ListView::self()->getSelectionAbilities());
}

void KEBApp::setModifiedFlag(bool modified) {
    m_modified = modified && !m_readOnly;

    QString caption = m_caption.isNull() ? QString::null : (m_caption + " ");
    if (m_bookmarksFilename 
     != KBookmarkManager::userBookmarksManager()->path())
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
        m_canPaste = KBookmarkDrag::canDecode(
                        kapp->clipboard()->data(QClipboard::Clipboard));
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

void KEBApp::slotConfigureToolbars() {
    saveMainWindowSettings(KGlobal::config(), "MainWindow");
    KEditToolbar dlg(actionCollection());
    connect(&dlg, SIGNAL( newToolbarConfig() ), 
                  SLOT( slotNewToolbarConfig() ));
    dlg.exec();
}

void KEBApp::slotNewToolbarConfig() {
    // called when OK or Apply is clicked
    createGUI();
    applyMainWindowSettings(KGlobal::config(), "MainWindow");
}

/* -------------------------- */

#include "toplevel.moc"

