/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <config-kmetadata.h>
#include "dolphinmainwindow.h"

#include <assert.h>

#include "dolphinapplication.h"
#include "dolphinnewmenu.h"
#include "dolphinsettings.h"
#include "dolphinsettingsdialog.h"
#include "dolphinstatusbar.h"
#include "dolphinapplication.h"
#include "urlnavigator.h"
#include "dolphinsettings.h"
#include "bookmarkssidebarpage.h"
#include "infosidebarpage.h"
#include "dolphin_generalsettings.h"
#include "viewpropertiesdialog.h"
#include "viewproperties.h"
#include "metadataloader.h"
#include "mainwindowadaptor.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kio/deletejob.h>
#include <kio/renamedialog.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <konqmimedata.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <ktoggleaction.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kurl.h>

#include <QCloseEvent>
#include <QClipboard>
#include <QSplitter>
#include <QDockWidget>

DolphinMainWindow::DolphinMainWindow(int id) :
    KMainWindow(0),
    m_newMenu(0),
    m_splitter(0),
    m_activeView(0),
    m_id(id)
{
    setObjectName("Dolphin");
    m_view[PrimaryIdx] = 0;
    m_view[SecondaryIdx] = 0;

    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QString("/dolphin/MainWindow%1").arg(m_id), this);

    KonqUndoManager::incRef();

    KonqUndoManager* undoManager = KonqUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface(this));

    connect(undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));
}

DolphinMainWindow::~DolphinMainWindow()
{
    KonqUndoManager::decRef();
    DolphinApplication::app()->removeMainWindow(this);
}

void DolphinMainWindow::setActiveView(DolphinView* view)
{
    assert((view == m_view[PrimaryIdx]) || (view == m_view[SecondaryIdx]));
    if (m_activeView == view) {
        return;
    }

    m_activeView = view;

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    setCaption(m_activeView->url().fileName());

    emit activeViewChanged();
}

void DolphinMainWindow::dropUrls(const KUrl::List& urls,
                                 const KUrl& destination)
{
    Qt::DropAction action = Qt::CopyAction;

    Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    const bool shiftPressed   = modifier & Qt::ShiftModifier;
    const bool controlPressed = modifier & Qt::ControlModifier;
    if (shiftPressed && controlPressed) {
        // shortcut for 'Link Here' is used
        action = Qt::LinkAction;
    }
    else if (shiftPressed) {
        // shortcut for 'Move Here' is used
        action = Qt::MoveAction;
    }
    else if (controlPressed) {
        // shortcut for 'Copy Here' is used
        action = Qt::CopyAction;
    }
    else {
        // open a context menu which offers the following actions:
        // - Move Here
        // - Copy Here
        // - Link Here
        // - Cancel

        KMenu popup(this);

        QString seq = QKeySequence(Qt::ShiftModifier).toString();
        seq.chop(1); // chop superfluous '+'
        QAction* moveAction = popup.addAction(KIcon("goto"),
                                              i18n("&Move Here") + '\t' + seq);

        seq = QKeySequence(Qt::ControlModifier).toString();
        seq.chop(1);
        QAction* copyAction = popup.addAction(KIcon("editcopy"),
                                              i18n("&Copy Here") + '\t' + seq);

        seq = QKeySequence(Qt::ControlModifier + Qt::ShiftModifier).toString();
        seq.chop(1);
        QAction* linkAction = popup.addAction(KIcon("www"),
                                              i18n("&Link Here") + '\t' + seq);

        popup.addSeparator();
        popup.addAction(KIcon("stop"), i18n("Cancel"));

        QAction* activatedAction = popup.exec(QCursor::pos());
        if (activatedAction == moveAction) {
            action = Qt::MoveAction;
        }
        else if (activatedAction == copyAction) {
            action = Qt::CopyAction;
        }
        else if (activatedAction == linkAction) {
            action = Qt::LinkAction;
        }
        else {
            return;
        }
    }

    switch (action) {
        case Qt::MoveAction:
            moveUrls(urls, destination);
            break;

        case Qt::CopyAction:
            copyUrls(urls, destination);
            break;

        case Qt::LinkAction:
            linkUrls(urls, destination);
            break;

        default:
            break;
    }
}

void DolphinMainWindow::refreshViews()
{
    const bool split = DolphinSettings::instance().generalSettings()->splitView();
    const bool isPrimaryViewActive = (m_activeView == m_view[PrimaryIdx]);
    KUrl url;
    for (int i = PrimaryIdx; i <= SecondaryIdx; ++i) {
       if (m_view[i] != 0) {
            url = m_view[i]->url();

            // delete view instance...
            m_view[i]->close();
            m_view[i]->deleteLater();
            m_view[i] = 0;
        }

        if (split || (i == PrimaryIdx)) {
            // ... and recreate it
            ViewProperties props(url);
            m_view[i] = new DolphinView(this,
                                        m_splitter,
                                        url,
                                        props.viewMode(),
                                        props.showHiddenFiles());
            connectViewSignals(i);
            m_view[i]->show();
        }
    }

    m_activeView = isPrimaryViewActive ? m_view[PrimaryIdx] : m_view[SecondaryIdx];
    assert(m_activeView != 0);

    updateViewActions();
    emit activeViewChanged();
}

void DolphinMainWindow::changeUrl(const QString& url)
{
    if (activeView() != 0) {
        activeView()->setUrl(KUrl(url));
    }
}

void DolphinMainWindow::slotViewModeChanged()
{
    updateViewActions();
}

void DolphinMainWindow::slotShowPreviewChanged()
{
    // It is not enough to update the 'Show Preview' action, also
    // the 'Zoom In' and 'Zoom Out' actions must be adapted.
    updateViewActions();
}

void DolphinMainWindow::slotShowHiddenFilesChanged()
{
    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(m_activeView->showHiddenFiles());
}

void DolphinMainWindow::slotSortingChanged(DolphinView::Sorting sorting)
{
    QAction* action = 0;
    switch (sorting) {
        case DolphinView::SortByName:
            action = actionCollection()->action("by_name");
            break;
        case DolphinView::SortBySize:
            action = actionCollection()->action("by_size");
            break;
        case DolphinView::SortByDate:
            action = actionCollection()->action("by_date");
            break;
        case DolphinView::SortByPermissions:
            action = actionCollection()->action("by_permissions");
            break;
        case DolphinView::SortByOwner:
            action = actionCollection()->action("by_owner");
            break;
        case DolphinView::SortByGroup:
            action = actionCollection()->action("by_group");
            break;
        default:
            break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }
}

void DolphinMainWindow::slotSortOrderChanged(Qt::SortOrder order)
{
    KToggleAction* descending = static_cast<KToggleAction*>(actionCollection()->action("descending"));
    const bool sortDescending = (order == Qt::Descending);
    descending->setChecked(sortDescending);
}

void DolphinMainWindow::slotSelectionChanged()
{
    updateEditActions();

    assert(m_view[PrimaryIdx] != 0);
    int selectedUrlsCount = m_view[PrimaryIdx]->selectedUrls().count();
    if (m_view[SecondaryIdx] != 0) {
        selectedUrlsCount += m_view[SecondaryIdx]->selectedUrls().count();
    }

    QAction* compareFilesAction = actionCollection()->action("compare_files");
    compareFilesAction->setEnabled(selectedUrlsCount == 2);

    m_activeView->updateStatusBar();

    emit selectionChanged();
}

void DolphinMainWindow::slotHistoryChanged()
{
    updateHistory();
}

void DolphinMainWindow::slotUrlChanged(const KUrl& url)
{
    updateEditActions();
    updateViewActions();
    updateGoActions();
    setCaption(url.fileName());
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    DolphinApplication::app()->createMainWindow()->show();
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();
    generalSettings->setFirstRun(false);

    settings.save();

    // TODO: I assume there will be a generic way in KDE 4 to store the docks
    // of the main window. In the meantime they are stored manually:
    QString filename = KStandardDirs::locateLocal("data", KGlobal::mainComponent().componentName());
    filename.append("/panels_layout");
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray data = saveState();
        file.write(data);
        file.close();
    }

    KMainWindow::closeEvent(event);
}

void DolphinMainWindow::saveProperties(KConfig* config)
{
    KConfigGroup primaryView = config->group("Primary view");
    primaryView.writeEntry("Url", m_view[PrimaryIdx]->url().url());
    primaryView.writeEntry("Editable Url", m_view[PrimaryIdx]->isUrlEditable());
    if (m_view[SecondaryIdx] != 0) {
        KConfigGroup secondaryView = config->group("Secondary view");
        secondaryView.writeEntry("Url", m_view[SecondaryIdx]->url().url());
        secondaryView.writeEntry("Editable Url", m_view[SecondaryIdx]->isUrlEditable());
    }
}

void DolphinMainWindow::readProperties(KConfig* config)
{
    const KConfigGroup primaryView = config->group("Primary view");
    m_view[PrimaryIdx]->setUrl(primaryView.readEntry("Url"));
    m_view[PrimaryIdx]->setUrlEditable(primaryView.readEntry("Editable Url", false));
    if (config->hasGroup("Secondary view")) {
        const KConfigGroup secondaryView = config->group("Secondary view");
        if (m_view[SecondaryIdx] == 0) {
            toggleSplitView();
        }
        m_view[SecondaryIdx]->setUrl(secondaryView.readEntry("Url"));
        m_view[SecondaryIdx]->setUrlEditable(secondaryView.readEntry("Editable Url", false));
    }
    else if (m_view[SecondaryIdx] != 0) {
        toggleSplitView();
    }
}

void DolphinMainWindow::updateNewMenu()
{
    m_newMenu->slotCheckUpToDate();
    m_newMenu->setPopupFiles(activeView()->url());
}

void DolphinMainWindow::rename()
{
    clearStatusBar();
    m_activeView->renameSelectedItems();
}

void DolphinMainWindow::moveToTrash()
{
    clearStatusBar();
    const KUrl::List selectedUrls = m_activeView->selectedUrls();
    KonqOperations::del(this, KonqOperations::TRASH, selectedUrls);
    m_undoOperations.append(KonqOperations::TRASH);
}

void DolphinMainWindow::deleteItems()
{
    clearStatusBar();

    KUrl::List list = m_activeView->selectedUrls();
    const uint itemCount = list.count();
    assert(itemCount >= 1);

    QString text;
    if (itemCount > 1) {
        text = i18n("Do you really want to delete the %1 selected items?",itemCount);
    }
    else {
        const KUrl& url = list.first();
        text = i18n("Do you really want to delete '%1'?",url.fileName());
    }

    const bool del = KMessageBox::warningContinueCancel(this,
                                                        text,
                                                        QString(),
                                                        KGuiItem(i18n("Delete"), KIcon("editdelete"))
                                                       ) == KMessageBox::Continue;
    if (del) {
        KIO::Job* job = KIO::del(list);
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotHandleJobError(KJob*)));
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotDeleteFileFinished(KJob*)));
    }
}

void DolphinMainWindow::properties()
{
    const KFileItemList list = m_activeView->selectedItems();
    new KPropertiesDialog(list, this);
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::slotHandleJobError(KJob* job)
{
    if (job->error() != 0) {
        DolphinStatusBar* statusBar = m_activeView->statusBar();
        statusBar->setMessage(job->errorString(),
                              DolphinStatusBar::Error);
    }
}

void DolphinMainWindow::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        DolphinStatusBar* statusBar = m_activeView->statusBar();
        statusBar->setMessage(i18n("Delete operation completed."),
                              DolphinStatusBar::OperationCompleted);
    }
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setEnabled(available);
    }

    if (available && (m_undoOperations.count() > 0)) {
        const KonqOperations::Operation op = m_undoOperations.takeFirst();
        DolphinStatusBar* statusBar = m_activeView->statusBar();
        switch (op) {
            case KonqOperations::COPY:
                statusBar->setMessage(i18n("Copy operation completed."),
                                      DolphinStatusBar::OperationCompleted);
                break;
            case KonqOperations::MOVE:
                statusBar->setMessage(i18n("Move operation completed."),
                                      DolphinStatusBar::OperationCompleted);
                break;
            case KonqOperations::LINK:
                statusBar->setMessage(i18n("Link operation completed."),
                                      DolphinStatusBar::OperationCompleted);
                break;
            case KonqOperations::TRASH:
                statusBar->setMessage(i18n("Move to trash operation completed."),
                                      DolphinStatusBar::OperationCompleted);
                break;
            default:
                break;
        }

    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString& text)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::undo()
{
    clearStatusBar();
    KonqUndoManager::self()->undo();
}

void DolphinMainWindow::cut()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = m_activeView->selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinMainWindow::copy()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = m_activeView->selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, false);

    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinMainWindow::paste()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    clearStatusBar();

    const KUrl::List sourceUrls = KUrl::List::fromMimeData(mimeData);

    // per default the pasting is done into the current Url of the view
    KUrl destUrl(m_activeView->url());

    // check whether the pasting should be done into a selected directory
    KUrl::List selectedUrls = m_activeView->selectedUrls();
    if (selectedUrls.count() == 1) {
        const KFileItem fileItem(S_IFDIR,
                                 KFileItem::Unknown,
                                 selectedUrls.first(),
                                 true);
        if (fileItem.isDir()) {
            // only one item is selected which is a directory, hence paste
            // into this directory
            destUrl = selectedUrls.first();
        }
    }

    if (KonqMimeData::decodeIsCutSelection(mimeData)) {
        moveUrls(sourceUrls, destUrl);
        clipboard->clear();
    }
    else {
        copyUrls(sourceUrls, destUrl);
    }
}

void DolphinMainWindow::updatePasteAction()
{
    QAction* pasteAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Paste));
    if (pasteAction == 0) {
        return;
    }

    QString text(i18n("Paste"));
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    if (!urls.isEmpty()) {
        pasteAction->setEnabled(true);

        const int count = urls.count();
        if (count == 1) {
            pasteAction->setText(i18n("Paste 1 File"));
        }
        else {
            pasteAction->setText(i18n("Paste %1 Files", count));
        }
    }
    else {
        pasteAction->setEnabled(false);
        pasteAction->setText(i18n("Paste"));
    }

    if (pasteAction->isEnabled()) {
        KUrl::List urls = m_activeView->selectedUrls();
        const uint count = urls.count();
        if (count > 1) {
            // pasting should not be allowed when more than one file
            // is selected
            pasteAction->setEnabled(false);
        }
        else if (count == 1) {
            // Only one file is selected. Pasting is only allowed if this
            // file is a directory.
            // TODO: this doesn't work with remote protocols; instead we need a
            // m_activeView->selectedFileItems() to get the real KFileItems
            const KFileItem fileItem(S_IFDIR,
                                     KFileItem::Unknown,
                                     urls.first(),
                                     true);
            pasteAction->setEnabled(fileItem.isDir());
        }
    }
}

void DolphinMainWindow::selectAll()
{
    clearStatusBar();
    m_activeView->selectAll();
}

void DolphinMainWindow::invertSelection()
{
    clearStatusBar();
    m_activeView->invertSelection();
}
void DolphinMainWindow::setIconsView()
{
    m_activeView->setMode(DolphinView::IconsView);
}

void DolphinMainWindow::setDetailsView()
{
    m_activeView->setMode(DolphinView::DetailsView);
}

void DolphinMainWindow::sortByName()
{
    m_activeView->setSorting(DolphinView::SortByName);
}

void DolphinMainWindow::sortBySize()
{
    m_activeView->setSorting(DolphinView::SortBySize);
}

void DolphinMainWindow::sortByDate()
{
    m_activeView->setSorting(DolphinView::SortByDate);
}

void DolphinMainWindow::sortByPermissions()
{
    m_activeView->setSorting(DolphinView::SortByPermissions);
}

void DolphinMainWindow::sortByOwner()
{
    m_activeView->setSorting(DolphinView::SortByOwner);
}

void DolphinMainWindow::sortByGroup()
{
    m_activeView->setSorting(DolphinView::SortByGroup);
}

void DolphinMainWindow::toggleSortOrder()
{
    const Qt::SortOrder order = (m_activeView->sortOrder() == Qt::Ascending) ?
                                Qt::Descending :
                                Qt::Ascending;
    m_activeView->setSortOrder(order);
}

void DolphinMainWindow::toggleSplitView()
{
    if (m_view[SecondaryIdx] == 0) {
        const int newWidth = (m_view[PrimaryIdx]->width() - m_splitter->handleWidth()) / 2;
        // create a secondary view
        m_view[SecondaryIdx] = new DolphinView(this,
                                               0,
                                               m_view[PrimaryIdx]->url(),
                                               m_view[PrimaryIdx]->mode(),
                                               m_view[PrimaryIdx]->showHiddenFiles());
        connectViewSignals(SecondaryIdx);
        m_splitter->addWidget(m_view[SecondaryIdx]);
        m_splitter->setSizes(QList<int>() << newWidth << newWidth);
        m_view[SecondaryIdx]->show();
    }
    else {
        // remove secondary view
        if (m_activeView == m_view[PrimaryIdx]) {
            m_view[SecondaryIdx]->close();
            m_view[SecondaryIdx]->deleteLater();
            m_view[SecondaryIdx] = 0;
            setActiveView(m_view[PrimaryIdx]);
        }
        else {
            // The secondary view is active, hence from the users point of view
            // the content of the secondary view should be moved to the primary view.
            // From an implementation point of view it is more efficient to close
            // the primary view and exchange the internal pointers afterwards.
            m_view[PrimaryIdx]->close();
            delete m_view[PrimaryIdx];
            m_view[PrimaryIdx] = m_view[SecondaryIdx];
            m_view[SecondaryIdx] = 0;
            setActiveView(m_view[PrimaryIdx]);
        }
    }
    emit activeViewChanged();
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    m_activeView->reload();
}

void DolphinMainWindow::stopLoading()
{
}

void DolphinMainWindow::togglePreview()
{
    clearStatusBar();

    const KToggleAction* showPreviewAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_preview"));
    const bool show = showPreviewAction->isChecked();
    m_activeView->setShowPreview(show);
}

void DolphinMainWindow::toggleShowHiddenFiles()
{
    clearStatusBar();

    const KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    const bool show = showHiddenFilesAction->isChecked();
    m_activeView->setShowHiddenFiles(show);
}

void DolphinMainWindow::showFilterBar()
{
    const KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    const bool show = showFilterBarAction->isChecked();
    m_activeView->showFilterBar(show);
}

void DolphinMainWindow::zoomIn()
{
    m_activeView->zoomIn();
    updateViewActions();
}

void DolphinMainWindow::zoomOut()
{
    m_activeView->zoomOut();
    updateViewActions();
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("editable_location"));

    bool editOrBrowse = action->isChecked();
    m_activeView->setUrlEditable(editOrBrowse);
}

void DolphinMainWindow::editLocation()
{
    m_activeView->setUrlEditable(true);
}

void DolphinMainWindow::adjustViewProperties()
{
    clearStatusBar();
    ViewPropertiesDialog dlg(m_activeView);
    dlg.exec();
}

void DolphinMainWindow::goBack()
{
    clearStatusBar();
    m_activeView->goBack();
}

void DolphinMainWindow::goForward()
{
    clearStatusBar();
    m_activeView->goForward();
}

void DolphinMainWindow::goUp()
{
    clearStatusBar();
    m_activeView->goUp();
}

void DolphinMainWindow::goHome()
{
    clearStatusBar();
    m_activeView->goHome();
}

void DolphinMainWindow::openTerminal()
{
    QString command("konsole --workdir \"");
    command.append(m_activeView->url().path());
    command.append('\"');

    KRun::runCommand(command, "Konsole", "konsole");
}

void DolphinMainWindow::findFile()
{
    KRun::run("kfind", m_activeView->url());
}

void DolphinMainWindow::compareFiles()
{
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view
    assert(m_view[PrimaryIdx] != 0);

    KUrl urlA;
    KUrl urlB;
    KUrl::List urls = m_view[PrimaryIdx]->selectedUrls();

    switch (urls.count()) {
        case 0: {
            assert(m_view[SecondaryIdx] != 0);
            urls = m_view[SecondaryIdx]->selectedUrls();
            assert(urls.count() == 2);
            urlA = urls[0];
            urlB = urls[1];
            break;
        }

        case 1: {
            urlA = urls[0];
            assert(m_view[SecondaryIdx] != 0);
            urls = m_view[SecondaryIdx]->selectedUrls();
            assert(urls.count() == 1);
            urlB = urls[0];
            break;
        }

        case 2: {
            urlA = urls[0];
            urlB = urls[1];
            break;
        }

        default: {
            // may not happen: compareFiles may only get invoked if 2
            // files are selected
            assert(false);
        }
    }

    QString command("kompare -c \"");
    command.append(urlA.pathOrUrl());
    command.append("\" \"");
    command.append(urlB.pathOrUrl());
    command.append('\"');
    KRun::runCommand(command, "Kompare", "kompare");

}

void DolphinMainWindow::editSettings()
{
    // TODO: make a static method for opening the settings dialog
    DolphinSettingsDialog dlg(this);
    dlg.exec();
}

void DolphinMainWindow::init()
{
    // Check whether Dolphin runs the first time. If yes then
    // a proper default window size is given at the end of DolphinMainWindow::init().
    GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    const bool firstRun = generalSettings->firstRun();
    if (firstRun) {
        generalSettings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    m_splitter = new QSplitter(this);

    DolphinSettings& settings = DolphinSettings::instance();

    KBookmarkManager* manager = settings.bookmarkManager();
    assert(manager != 0);
    KBookmarkGroup root = manager->root();
    if (root.first().isNull()) {
        root.addBookmark(manager, i18n("Home"), settings.generalSettings()->homeUrl(), "folder_home");
        root.addBookmark(manager, i18n("Storage Media"), KUrl("media:/"), "blockdevice");
        root.addBookmark(manager, i18n("Network"), KUrl("remote:/"), "network_local");
        root.addBookmark(manager, i18n("Root"), KUrl("/"), "folder_red");
        root.addBookmark(manager, i18n("Trash"), KUrl("trash:/"), "trashcan_full");
    }

    setupActions();

    const KUrl& homeUrl = settings.generalSettings()->homeUrl();
    setCaption(homeUrl.fileName());
    ViewProperties props(homeUrl);
    m_view[PrimaryIdx] = new DolphinView(this,
                                         m_splitter,
                                         homeUrl,
                                         props.viewMode(),
                                         props.showHiddenFiles());
    connectViewSignals(PrimaryIdx);
    m_view[PrimaryIdx]->show();

    m_activeView = m_view[PrimaryIdx];

    setCentralWidget(m_splitter);
    setupDockWidgets();

    setupGUI(Keys|Save|Create|ToolBar);
    createGUI();

    stateChanged("new_file");
    setAutoSaveSettings();

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));
    updatePasteAction();
    updateGoActions();

    loadSettings();

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(640, 480);
    }
    #ifdef HAVE_KMETADATA
    if (!DolphinApplication::app()->metadataLoader()->storageUp())
        activeView()->statusBar()->setMessage(i18n("Failed to contact Nepomuk service, annotation and tagging are disabled."), DolphinStatusBar::Error);
    #endif
}

void DolphinMainWindow::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    KToggleAction* splitAction = static_cast<KToggleAction*>(actionCollection()->action("split_view"));
    if (settings->splitView()) {
        splitAction->setChecked(true);
        toggleSplitView();
    }

    updateViewActions();

    // TODO: I assume there will be a generic way in KDE 4 to restore the docks
    // of the main window. In the meantime they are restored manually (see also
    // DolphinMainWindow::closeEvent() for more details):
    QString filename = KStandardDirs::locateLocal("data", KGlobal::mainComponent().componentName());   filename.append("/panels_layout");
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        restoreState(data);
        file.close();
    }
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newMenu = new DolphinNewMenu(this);
    KMenu* menu = m_newMenu->menu();
    menu->setTitle(i18n("Create New..."));
    menu->setIcon(SmallIcon("filenew"));
    connect(menu, SIGNAL(aboutToShow()),
            this, SLOT(updateNewMenu()));

    QAction* newWindow = actionCollection()->addAction("new_window");
    newWindow->setIcon(KIcon("window_new"));
    newWindow->setText(i18n("New &Window"));
    newWindow->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(newWindow, SIGNAL(triggered()), this, SLOT(openNewMainWindow()));

    QAction* rename = actionCollection()->addAction("rename");
    rename->setText(i18n("Rename"));
    rename->setShortcut(Qt::Key_F2);
    connect(rename, SIGNAL(triggered()), this, SLOT(rename()));

    QAction* moveToTrash = actionCollection()->addAction("move_to_trash");
    moveToTrash->setText(i18n("Move to Trash"));
    moveToTrash->setIcon(KIcon("edittrash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered()), this, SLOT(moveToTrash()));

    QAction* deleteAction = actionCollection()->addAction("delete");
    deleteAction->setText(i18n("Delete"));
    deleteAction->setShortcut(Qt::SHIFT | Qt::Key_Delete);
    deleteAction->setIcon(KIcon("editdelete"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItems()));

    QAction* properties = actionCollection()->addAction("properties");
    properties->setText(i18n("Properties"));
    properties->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(properties, SIGNAL(triggered()), this, SLOT(properties()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          SLOT(undo()),
                          actionCollection());

    KStandardAction::cut(this, SLOT(cut()), actionCollection());
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KStandardAction::paste(this, SLOT(paste()), actionCollection());

    QAction* selectAll = actionCollection()->addAction("select_all");
    selectAll->setText(i18n("Select All"));
    selectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    QAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18n("Invert Selection"));
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), this, SLOT(invertSelection()));

    // setup 'View' menu
    KStandardAction::zoomIn(this,
                       SLOT(zoomIn()),
                       actionCollection());

    KStandardAction::zoomOut(this,
                        SLOT(zoomOut()),
                        actionCollection());

    KToggleAction* iconsView = actionCollection()->add<KToggleAction>("icons");
    iconsView->setText(i18n("Icons"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("view_icon"));
    connect(iconsView, SIGNAL(triggered()), this, SLOT(setIconsView()));

    KToggleAction* detailsView = actionCollection()->add<KToggleAction>("details");
    detailsView->setText(i18n("Details"));
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("view_text"));
    connect(detailsView, SIGNAL(triggered()), this, SLOT(setDetailsView()));

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(iconsView);
    viewModeGroup->addAction(detailsView);

    KToggleAction* sortByName = actionCollection()->add<KToggleAction>("by_name");
    sortByName->setText(i18n("By Name"));
    connect(sortByName, SIGNAL(triggered()), this, SLOT(sortByName()));

    KToggleAction* sortBySize = actionCollection()->add<KToggleAction>("by_size");
    sortBySize->setText(i18n("By Size"));
    connect(sortBySize, SIGNAL(triggered()), this, SLOT(sortBySize()));

    KToggleAction* sortByDate = actionCollection()->add<KToggleAction>("by_date");
    sortByDate->setText(i18n("By Date"));
    connect(sortByDate, SIGNAL(triggered()), this, SLOT(sortByDate()));

    KToggleAction* sortByPermissions = actionCollection()->add<KToggleAction>("by_permissions");
    sortByPermissions->setText(i18n("By Permissions"));
    connect(sortByPermissions, SIGNAL(triggered()), this, SLOT(sortByPermissions()));

    KToggleAction* sortByOwner = actionCollection()->add<KToggleAction>("by_owner");
    sortByOwner->setText(i18n("By Owner"));
    connect(sortByOwner, SIGNAL(triggered()), this, SLOT(sortByOwner()));

    KToggleAction* sortByGroup = actionCollection()->add<KToggleAction>("by_group");
    sortByGroup->setText(i18n("By Group"));
    connect(sortByGroup, SIGNAL(triggered()), this, SLOT(sortByGroup()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->addAction(sortByName);
    sortGroup->addAction(sortBySize);
    sortGroup->addAction(sortByDate);
    sortGroup->addAction(sortByPermissions);
    sortGroup->addAction(sortByOwner);
    sortGroup->addAction(sortByGroup);

    KToggleAction* sortDescending = actionCollection()->add<KToggleAction>("descending");
    sortDescending->setText(i18n("Descending"));
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    KToggleAction* showPreview = actionCollection()->add<KToggleAction>("show_preview");
    showPreview->setText(i18n("Show Preview"));
    connect(showPreview, SIGNAL(triggered()), this, SLOT(togglePreview()));

    KToggleAction* showHiddenFiles = actionCollection()->add<KToggleAction>("show_hidden_files");
    showHiddenFiles->setText(i18n("Show Hidden Files"));
    showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_Period);
    connect(showHiddenFiles, SIGNAL(triggered()), this, SLOT(toggleShowHiddenFiles()));

    KToggleAction* split = actionCollection()->add<KToggleAction>("split_view");
    split->setText(i18n("Split View"));
    split->setShortcut(Qt::Key_F10);
    split->setIcon(KIcon("view_left_right"));
    connect(split, SIGNAL(triggered()), this, SLOT(toggleSplitView()));

    QAction* reload = actionCollection()->addAction("reload");
    reload->setText(i18n("Reload"));
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(KIcon("reload"));
    connect(reload, SIGNAL(triggered()), this, SLOT(reloadView()));

    QAction* stop = actionCollection()->addAction("stop");
    stop->setText(i18n("Stop"));
    stop->setIcon(KIcon("stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    // TODO: the URL navigator must emit a signal if the editable state has been
    // changed, so that the corresponding showFullLocation action is updated. Also
    // the naming "Show full Location" is currently confusing...
    KToggleAction* showFullLocation = actionCollection()->add<KToggleAction>("editable_location");
    showFullLocation->setText(i18n("Show Full Location"));
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    QAction* editLocation = actionCollection()->addAction("edit_location");
    editLocation->setText(i18n("Edit Location"));
    editLocation->setShortcut(Qt::Key_F6);
    connect(editLocation, SIGNAL(triggered()), this, SLOT(editLocation()));

    QAction* adjustViewProps = actionCollection()->addAction("view_properties");
    adjustViewProps->setText(i18n("Adjust View Properties..."));
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(adjustViewProperties()));

    // setup 'Go' menu
    KStandardAction::back(this, SLOT(goBack()), actionCollection());
    KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    KStandardAction::up(this, SLOT(goUp()), actionCollection());
    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    QAction* openTerminal = actionCollection()->addAction("open_terminal");
    openTerminal->setText(i18n("Open Terminal"));
    openTerminal->setShortcut(Qt::Key_F4);
    openTerminal->setIcon(KIcon("konsole"));
    connect(openTerminal, SIGNAL(triggered()), this, SLOT(openTerminal()));

    QAction* findFile = actionCollection()->addAction("find_file");
    findFile->setText(i18n("Find File..."));
    findFile->setShortcut(Qt::Key_F);
    findFile->setIcon(KIcon("filefind"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(findFile()));

    KToggleAction* showFilterBar = actionCollection()->add<KToggleAction>("show_filter_bar");
    showFilterBar->setText(i18n("Show Filter Bar"));
    showFilterBar->setShortcut(Qt::Key_Slash);
    connect(showFilterBar, SIGNAL(triggered()), this, SLOT(showFilterBar()));

    QAction* compareFiles = actionCollection()->addAction("compare_files");
    compareFiles->setText(i18n("Compare Files"));
    compareFiles->setIcon(KIcon("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, SIGNAL(triggered()), this, SLOT(compareFiles()));

    // setup 'Settings' menu
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());
}

void DolphinMainWindow::setupDockWidgets()
{
    QDockWidget* shortcutsDock = new QDockWidget(i18n("Bookmarks"));
    shortcutsDock->setObjectName("bookmarksDock");
    shortcutsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    shortcutsDock->setWidget(new BookmarksSidebarPage(this));

    shortcutsDock->toggleViewAction()->setText(i18n("Show Bookmarks Panel"));
    actionCollection()->addAction("show_bookmarks_panel", shortcutsDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, shortcutsDock);

    QDockWidget* infoDock = new QDockWidget(i18n("Information"));
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    infoDock->setWidget(new InfoSidebarPage(this));

    infoDock->toggleViewAction()->setText(i18n("Show Information Panel"));
    actionCollection()->addAction("show_info_panel", infoDock->toggleViewAction());

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
}

void DolphinMainWindow::updateHistory()
{
    int index = 0;
    const QLinkedList<UrlNavigator::HistoryElem> list = m_activeView->urlHistory(index);

    QAction* backAction = actionCollection()->action("go_back");
    if (backAction != 0) {
        backAction->setEnabled(index < static_cast<int>(list.count()) - 1);
    }

    QAction* forwardAction = actionCollection()->action("go_forward");
    if (forwardAction != 0) {
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateEditActions()
{
    const KFileItemList list = m_activeView->selectedItems();
    if (list.isEmpty()) {
        stateChanged("has_no_selection");
    }
    else {
        stateChanged("has_selection");

        QAction* renameAction = actionCollection()->action("rename");
        if (renameAction != 0) {
            renameAction->setEnabled(list.count() >= 1);
        }

        bool enableMoveToTrash = true;

        KFileItemList::const_iterator it = list.begin();
        const KFileItemList::const_iterator end = list.end();
        while (it != end) {
            KFileItem* item = *it;
            const KUrl& url = item->url();
            // only enable the 'Move to Trash' action for local files
            if (!url.isLocalFile()) {
                enableMoveToTrash = false;
            }
            ++it;
        }

        QAction* moveToTrashAction = actionCollection()->action("move_to_trash");
        moveToTrashAction->setEnabled(enableMoveToTrash);
    }
    updatePasteAction();
}

void DolphinMainWindow::updateViewActions()
{
    QAction* zoomInAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::ZoomIn));
    if (zoomInAction != 0) {
        zoomInAction->setEnabled(m_activeView->isZoomInPossible());
    }

    QAction* zoomOutAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::ZoomOut));
    if (zoomOutAction != 0) {
        zoomOutAction->setEnabled(m_activeView->isZoomOutPossible());
    }

    QAction* action = 0;
    switch (m_activeView->mode()) {
        case DolphinView::IconsView:
            action = actionCollection()->action("icons");
            break;
        case DolphinView::DetailsView:
            action = actionCollection()->action("details");
            break;
        default:
            break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }

    slotSortingChanged(m_activeView->sorting());
    slotSortOrderChanged(m_activeView->sortOrder());

    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(m_activeView->isFilterBarVisible());

    KToggleAction* showPreviewAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_preview"));
    showPreviewAction->setChecked(m_activeView->showPreview());

    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(m_activeView->showHiddenFiles());

    KToggleAction* splitAction = static_cast<KToggleAction*>(actionCollection()->action("split_view"));
    splitAction->setChecked(m_view[SecondaryIdx] != 0);

    KToggleAction* editableLocactionAction =
        static_cast<KToggleAction*>(actionCollection()->action("editable_location"));
    editableLocactionAction->setChecked(m_activeView->isUrlEditable());
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Up));
    const KUrl& currentUrl = m_activeView->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::copyUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::COPY, source, dest);
    m_undoOperations.append(KonqOperations::COPY);
}

void DolphinMainWindow::moveUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::MOVE, source, dest);
    m_undoOperations.append(KonqOperations::MOVE);
}

void DolphinMainWindow::linkUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::LINK, source, dest);
    m_undoOperations.append(KonqOperations::LINK);
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeView->statusBar()->clear();
}

void DolphinMainWindow::connectViewSignals(int viewIndex)
{
    DolphinView* view = m_view[viewIndex];
    connect(view, SIGNAL(modeChanged()),
            this, SLOT(slotViewModeChanged()));
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
    connect(view, SIGNAL(showHiddenFilesChanged()),
            this, SLOT(slotShowHiddenFilesChanged()));
    connect(view, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(slotSortingChanged(DolphinView::Sorting)));
    connect(view, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(slotSortOrderChanged(Qt::SortOrder)));
    connect(view, SIGNAL(selectionChanged()),
            this, SLOT(slotSelectionChanged()));
    connect(view, SIGNAL(showFilterBarChanged(bool)),
            this, SLOT(updateFilterBarAction(bool)));

    const UrlNavigator* navigator = view->urlNavigator();
    connect(navigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(slotUrlChanged(const KUrl&)));
    connect(navigator, SIGNAL(historyChanged()),
            this, SLOT(slotHistoryChanged()));

}

DolphinMainWindow::UndoUiInterface::UndoUiInterface(DolphinMainWindow* mainWin) :
    KonqUndoManager::UiInterface(mainWin),
    m_mainWin(mainWin)
{
    assert(m_mainWin != 0);
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface()
{
}

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job* job)
{
    DolphinStatusBar* statusBar = m_mainWin->activeView()->statusBar();
    statusBar->setMessage(job->errorString(), DolphinStatusBar::Error);
}

#include "dolphinmainwindow.moc"
