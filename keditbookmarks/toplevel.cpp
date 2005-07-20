// -*- indent-tabs-mode:nil -*-
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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toplevel.h"

#include "bookmarkinfo.h"
#include "listview.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "exporters.h"
#include "settings.h"
#include "commands.h"
#include "kebsearchline.h"

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

#include <kdebug.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

CmdHistory* CmdHistory::s_self = 0;

CmdHistory::CmdHistory(KActionCollection *collection)
    : m_commandHistory(collection) {
    connect(&m_commandHistory, SIGNAL( commandExecuted(KCommand *) ),
            SLOT( slotCommandExecuted(KCommand *) ));
    assert(!s_self);
    s_self = this; // this is hacky
}

CmdHistory* CmdHistory::self() {
    assert(s_self);
    return s_self;
}

void CmdHistory::slotCommandExecuted(KCommand *k) {
    KEBApp::self()->notifyCommandExecuted();

    IKEBCommand * cmd = dynamic_cast<IKEBCommand *>(k);
    Q_ASSERT(cmd);

    KBookmark bk = CurrentMgr::bookmarkAt(cmd->affectedBookmarks());
    Q_ASSERT(bk.isGroup());
    CurrentMgr::self()->notifyManagers(bk.toGroup());

    // sets currentItem to something sensible
    // if the currentItem was invalidated by executing
    // CreateCommand or DeleteManyCommand
    // otherwise does nothing
    // sensible is either a already selected item or cmd->currentAddress()
    ListView::self()->fixUpCurrent( cmd->currentAddress() );
}

void CmdHistory::notifyDocSaved() {
    m_commandHistory.documentSaved();
}

void CmdHistory::didCommand(KCommand *cmd) {
    if (!cmd)
        return;
    m_commandHistory.addCommand(cmd, false);
    CmdHistory::slotCommandExecuted(cmd);
}

void CmdHistory::addCommand(KCommand *cmd) {
    if (!cmd)
        return;
    m_commandHistory.addCommand(cmd);
}

void CmdHistory::addInFlightCommand(KCommand *cmd)
{
    if(!cmd)
        return;
    m_commandHistory.addCommand(cmd, false);
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
        kdDebug()<<"ERROR calling createManager twice"<<endl;
        disconnect(m_mgr, 0, 0, 0);
        // still todo - delete old m_mgr
    }

    m_mgr = KBookmarkManager::managerForFile(filename, false);

    connect(m_mgr, SIGNAL( changed(const QString &, const QString &) ),
            SLOT( slotBookmarksChanged(const QString &, const QString &) ));
}

void CurrentMgr::slotBookmarksChanged(const QString &, const QString &) {
    if(ignorenext > 0) //We ignore the first changed signal after every change we did
    {
        --ignorenext;
        return;
    }

    CmdHistory::self()->clearHistory();
    ListView::self()->updateListView();
    KEBApp::self()->updateActions();
}

void CurrentMgr::notifyManagers(KBookmarkGroup grp)
{
    ++ignorenext;
    mgr()->emitChanged(grp);
}

void CurrentMgr::notifyManagers() {
    notifyManagers( mgr()->root() );
}

void CurrentMgr::reloadConfig() {
    mgr()->emitConfigChanged();
}

QString CurrentMgr::makeTimeStr(const QString & in)
{
    int secs;
    bool ok;
    secs = in.toInt(&ok);
    if (!ok)
        return QString::null;
    return makeTimeStr(secs);
}

QString CurrentMgr::makeTimeStr(int b)
{
    QDateTime dt;
    dt.setTime_t(b);
    return (dt.daysTo(QDateTime::currentDateTime()) > 31)
        ? KGlobal::locale()->formatDate(dt.date(), false)
        : KGlobal::locale()->formatDateTime(dt, false);
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
    resetQuickSearch->setWhatsThis( i18n( "<b>Reset Quick Search</b><br>"
        "Resets the quick search so that all bookmarks are shown again." ) );
    resetQuickSearch->plug( quicksearch );

    QLabel *lbl = new QLabel(i18n("Se&arch:"), quicksearch, "kde toolbar widget");

    KListViewSearchLine *searchLineEdit = new KEBSearchLine(quicksearch, 0, "KListViewSearchLine");
    quicksearch->setStretchableWidget(searchLineEdit);
    lbl->setBuddy(searchLineEdit);
    connect(resetQuickSearch, SIGNAL(activated()), searchLineEdit, SLOT(clear()));

    ListView::createListViews(vsplitter);
    ListView::self()->initListViews();
    searchLineEdit->setListView(static_cast<KListView*>(ListView::self()->widget()));
    ListView::self()->setSearchLine(searchLineEdit);

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

    ListView::self()->setCurrent(ListView::self()->getItemAtAddress(address), true);

    setCancelFavIconUpdatesEnabled(false);
    setCancelTestsEnabled(false);
    updateActions();
}

void KEBApp::construct() {
    CurrentMgr::self()->createManager(m_bookmarksFilename);

    ListView::self()->updateListViewSetup(m_readOnly);
    ListView::self()->updateListView();
    ListView::self()->widget()->setFocus();

    slotClipboardDataChanged();
    setAutoSaveSettings();
}

void KEBApp::updateStatus(QString url)
{
    if(m_bkinfo->bookmark().url() == url)
        m_bkinfo->updateStatus();
}

KEBApp::~KEBApp() {
    s_topLevel = 0;
    delete m_cmdHistory;
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

    getToggleAction("settings_showNS")
        ->setChecked(CurrentMgr::self()->showNSBookmarks());
}

bool KEBApp::nsShown() const {
    return getToggleAction("settings_showNS")->isChecked();
}

// this should be pushed from listview, not pulled
void KEBApp::updateActions() {
    resetActions();
    setActionsEnabled(ListView::self()->getSelectionAbilities());
}

void KEBApp::slotClipboardDataChanged() {
    // kdDebug() << "KEBApp::slotClipboardDataChanged" << endl;
    if (!m_readOnly) {
        m_canPaste = KBookmarkDrag::canDecode(
                        kapp->clipboard()->data(QClipboard::Clipboard));
        updateActions();
    }
}

/* -------------------------- */

void KEBApp::notifyCommandExecuted() {
    // kdDebug() << "KEBApp::notifyCommandExecuted()" << endl;
    if (!m_readOnly) {        
        ListView::self()->updateListView();
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

