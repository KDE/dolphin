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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toplevel.h"

#include "bookmarkmodel.h"

#include "bookmarkinfo.h"
#include "actionsimpl.h"
#include "dcop.h"
#include "exporters.h"
#include "settings.h"
#include "commands.h"
#include "kebsearchline.h"
#include "bookmarklistview.h"

#include <stdlib.h>

#include <QClipboard>
#include <QSplitter>
#include <QLayout>
#include <QLabel>

#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kapplication.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaction.h>

#include <assert.h>
#include <qglobal.h>



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

KBookmarkGroup CurrentMgr::root()
{
    return mgr()->root();
}

KBookmark CurrentMgr::bookmarkAt(const QString &a)
{
    return self()->mgr()->findByAddress(a);
}

bool CurrentMgr::managerSave() { return mgr()->save(); }
void CurrentMgr::saveAs(const QString &fileName) { mgr()->saveAs(fileName); }
void CurrentMgr::setUpdate(bool update) { mgr()->setUpdate(update); }
QString CurrentMgr::path() const { return mgr()->path(); }

void CurrentMgr::createManager(const QString &filename, const QString &dbusObjectName) {
    if (m_mgr) {
        kDebug()<<"ERROR calling createManager twice"<<endl;
        disconnect(m_mgr, 0, 0, 0);
        // still todo - delete old m_mgr
    }

    kDebug()<<"DBus Object name: "<<dbusObjectName<<endl;
    m_mgr = KBookmarkManager::managerForFile(filename, dbusObjectName, false);

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
    KEBApp::self()->updateActions();
}

void CurrentMgr::notifyManagers(KBookmarkGroup grp)
{
    ++ignorenext;
    mgr()->emitChanged(grp);
}

void CurrentMgr::notifyManagers() {
    notifyManagers( root() );
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
        return QString();
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
    const QString &address, bool browser, const QString &caption,
    const QString &dbusObjectName
) : KMainWindow(), m_dcopIface(0), m_bookmarksFilename(bookmarksFile),
    m_caption(caption), m_readOnly(readonly), m_browser(browser), 
    m_dbusObjectName(dbusObjectName)
 {

    Q_UNUSED(address);//FIXME sets the current item

    m_cmdHistory = new CmdHistory(actionCollection());

    s_topLevel = this;

    QSplitter *vsplitter = new QSplitter(this);

    createActions();
    if (m_browser)
        createGUI();
    else
        createGUI("keditbookmarks-genui.rc");

    m_dcopIface = new KBookmarkEditorIface();

    connect(qApp->clipboard(), SIGNAL( dataChanged() ),
                               SLOT( slotClipboardDataChanged() ));

    KGlobal::locale()->insertCatalog("libkonq");

    m_canPaste = false;

    CurrentMgr::self()->createManager(m_bookmarksFilename, m_dbusObjectName);

    mBookmarkListView = new BookmarkListView();
    mBookmarkListView->setModel( BookmarkModel::self() );
    mBookmarkListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mBookmarkListView->loadColumnSetting();

    KToolBar * toolbar = new KToolBar(0L, "search toolbar");
    new KViewSearchLineWidget(mBookmarkListView, toolbar);

    mBookmarkFolderView = new BookmarkFolderView(mBookmarkListView);

    m_bkinfo = new BookmarkInfoWidget(mBookmarkListView);

    vsplitter->setOrientation(Qt::Vertical);
    vsplitter->addWidget(toolbar);
    vsplitter->addWidget(mBookmarkListView);
    vsplitter->addWidget(m_bkinfo);

    QSplitter *hsplitter = new QSplitter(this);
    hsplitter->setOrientation(Qt::Horizontal);
    hsplitter->addWidget(mBookmarkFolderView);
    hsplitter->addWidget(vsplitter);
    //FIXME set sensible sizes for vsplitter and hsplitter

    setCentralWidget(hsplitter);

    expandAll();

    slotClipboardDataChanged();
    setAutoSaveSettings();

    connect(mBookmarkListView->selectionModel(), SIGNAL(selectionChanged(  const QItemSelection &, const QItemSelection & )),
            this, SLOT(selectionChanged()));

    setCancelFavIconUpdatesEnabled(false);
    setCancelTestsEnabled(false);
    updateActions();
}

void KEBApp::reset(const QString & caption, const QString & bookmarksFileName)
{
    m_caption = caption;
    m_bookmarksFilename = bookmarksFileName;
    CurrentMgr::self()->createManager(m_bookmarksFilename, m_dbusObjectName); //FIXME this is still a memory leak (iff called by slotLoad)
    BookmarkModel::self()->resetModel();
    expandAll();
    updateActions();
}

void KEBApp::collapseAll()
{
    collapseAllHelper( BookmarkModel::self()->index(0, 0, QModelIndex()));
}

void KEBApp::collapseAllHelper( QModelIndex index )
{
    mBookmarkListView->collapse(index);
    int rowCount = index.model()->rowCount(index);
    for(int i=0; i<rowCount; ++i)
        collapseAllHelper(index.child(i, 0));
}

void KEBApp::expandAll()
{
    expandAllHelper( mBookmarkListView, BookmarkModel::self()->index(0, 0, QModelIndex()));
    expandAllHelper( mBookmarkFolderView, BookmarkModel::self()->index(0, 0, QModelIndex()));
}

void KEBApp::expandAllHelper(QTreeView * view, QModelIndex index)
{
    view->expand(index);
    int rowCount = index.model()->rowCount(index);
    for(int i=0; i<rowCount; ++i)
        expandAllHelper(view, index.child(i, 0));
}

void KEBApp::startEdit( Column c )
{
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    QModelIndexList::const_iterator it, end;
    end = list.constEnd();
    for(it = list.constBegin(); it != end; ++it)
        if( (*it).column() == int(c) && (BookmarkModel::self()->flags(*it) & Qt::ItemIsEditable) )
            return mBookmarkListView->edit( *it );
}

KBookmark KEBApp::firstSelected() const
{
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    QModelIndex index = *list.constBegin();
    return static_cast<TreeItem *>(index.internalPointer())->bookmark();
}

QString KEBApp::insertAddress() const
{
    KBookmark current = firstSelected();
    return (current.isGroup())
        ? (current.address() + "/0") //FIXME internal represantation used
        : KBookmark::nextAddress(current.address());
}

bool lessAddress(QString a, QString b)
{
    if(a == b)
         return false;

    QString error("ERROR");
    if(a == error)
        return false;
    if(b == error)
        return true;

    a += '/';
    b += '/';

    uint aLast = 0;
    uint bLast = 0;
    uint aEnd = a.length();
    uint bEnd = b.length();
    // Each iteration checks one "/"-delimeted part of the address
    // "" is treated correctly
    while(true)
    {
        // Invariant: a[0 ... aLast] == b[0 ... bLast]
        if(aLast + 1 == aEnd) //The last position was the last slash
            return true; // That means a is shorter than b
        if(bLast +1 == bEnd)
            return false;

        uint aNext = a.indexOf("/", aLast + 1);
        uint bNext = b.indexOf("/", bLast + 1);

        bool okay;
        uint aNum = a.mid(aLast + 1, aNext - aLast - 1).toUInt(&okay);
        if(!okay)
            return false;
        uint bNum = b.mid(bLast + 1, bNext - bLast - 1).toUInt(&okay);
        if(!okay)
            return true;

        if(aNum != bNum)
            return aNum < bNum;

        aLast = aNext;
        bLast = bNext;
    }
}

bool lessBookmark(const KBookmark & first, const KBookmark & second) //FIXME Using internal represantation
{
    return lessAddress(first.address(), second.address());
}

KBookmark::List KEBApp::selectedBookmarks() const
{
    KBookmark::List bookmarks;
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    QModelIndexList::const_iterator it, end;
    end = list.constEnd();
    for( it = list.constBegin(); it != end; ++it)
    {
        if((*it).column() != 0)
            continue;
        KBookmark bk = static_cast<TreeItem *>((*it).internalPointer())->bookmark();;
        if(bk.address() != CurrentMgr::self()->root().address())
            bookmarks.push_back( bk );
    }
    qSort(bookmarks.begin(), bookmarks.end(), lessBookmark);
    return bookmarks;
}

KBookmark::List KEBApp::selectedBookmarksExpanded() const
{
    KBookmark::List bookmarks = selectedBookmarks();
    KBookmark::List result;
    KBookmark::List::const_iterator it, end;
    end = bookmarks.constEnd();
    for(it = bookmarks.constBegin(); it != end; ++it)
    {
        selectedBookmarksExpandedHelper( *it, result );
    }
    return result;
}

void KEBApp::selectedBookmarksExpandedHelper(KBookmark bk, KBookmark::List & bookmarks) const
{
    bookmarks.push_back( bk );
    if(bk.isGroup())
    {
        KBookmarkGroup parent = bk.toGroup();
        KBookmark child = parent.first();
        while(child.hasParent())
        {
            selectedBookmarksExpandedHelper(child, bookmarks);
            child = parent.next(child);
        }
    }
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
    delete mBookmarkListView;
    delete BookmarkModel::self();
}

KToggleAction* KEBApp::getToggleAction(const char *action) const {
    return static_cast<KToggleAction*>(actionCollection()->action(action));
}

void KEBApp::resetActions() {
    stateChanged("disablestuff");
    stateChanged("normal");

    if (!m_readOnly)
        stateChanged("notreadonly");
}

void  KEBApp::selectionChanged()
{
    updateActions();
}

void KEBApp::updateActions() {
    // FIXME if nothing is selected in the item view, the folder in the group view
    // is selected. 
    // Change setActionsEnabled() and firstSelected() to match that
    resetActions();
    setActionsEnabled(mBookmarkListView->getSelectionAbilities());
}

void KEBApp::slotClipboardDataChanged() {
    // kDebug() << "KEBApp::slotClipboardDataChanged" << endl;
    if (!m_readOnly) {
        m_canPaste = KBookmark::List::canDecode(
                        QApplication::clipboard()->mimeData());
        updateActions();
    }
}

/* -------------------------- */

void KEBApp::notifyCommandExecuted() {
    // kDebug() << "KEBApp::notifyCommandExecuted()" << endl;
    updateActions();
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

