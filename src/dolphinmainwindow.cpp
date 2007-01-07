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

#include "dolphinmainwindow.h"

#include <assert.h>

#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kbookmarkmanager.h>
#include <kglobal.h>
#include <kpropertiesdialog.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kstatusbar.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kurl.h>
#include <kaction.h>
#include <kstandardaction.h>
#include <kmenu.h>
#include <kio/renamedialog.h>
#include <kinputdialog.h>
#include <kshell.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <klocale.h>
#include <konqmimedata.h>

#include <qclipboard.h>
#include <q3dragobject.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QCloseEvent>
#include <QSplitter>
#include <QDockWidget>

#include "urlnavigator.h"
#include "viewpropertiesdialog.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphinsettingsdialog.h"
#include "dolphinstatusbar.h"
#include "dolphinapplication.h"
#include "undomanager.h"
#include "progressindicator.h"
#include "dolphinsettings.h"
#include "bookmarkssidebarpage.h"
#include "infosidebarpage.h"
#include "generalsettings.h"
#include "dolphinapplication.h"


DolphinMainWindow::DolphinMainWindow() :
    KMainWindow(0),
    m_splitter(0),
    m_activeView(0)
{
    setObjectName("Dolphin");
    m_view[PrimaryIdx] = 0;
    m_view[SecondaryIdx] = 0;
}

DolphinMainWindow::~DolphinMainWindow()
{
    qDeleteAll(m_fileGroupActions);
    m_fileGroupActions.clear();

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
    m_dropDestination = destination;
    m_droppedUrls = urls;

    /* KDE4-TODO
    const ButtonState keyboardState = KApplication::keyboardMouseState();
    const bool shiftPressed = (keyboardState & ShiftButton) > 0;
    const bool controlPressed = (keyboardState & ControlButton) > 0;



    if (shiftPressed && controlPressed) {
        // shortcut for 'Linke Here' is used
        selectedIndex = 2;
    }
    else if (controlPressed) {
        // shortcut for 'Copy Here' is used
        selectedIndex = 1;
    }
    else if (shiftPressed) {
        // shortcut for 'Move Here' is used
        selectedIndex = 0;
    }
    else*/ {
        // no shortcut is used, hence open a popup menu
        KMenu popup(this);

        QAction* moveAction = popup.addAction(SmallIcon("goto"), i18n("&Move Here"));
        connect(moveAction, SIGNAL(triggered()), this, SLOT(moveDroppedItems()));

        QAction* copyAction = popup.addAction(SmallIcon("editcopy"), i18n( "&Copy Here" ));
        connect(copyAction, SIGNAL(triggered()), this, SLOT(copyDroppedItems()));

        QAction* linkAction = popup.addAction(i18n("&Link Here"));
        connect(linkAction, SIGNAL(triggered()), this, SLOT(linkDroppedItems()));

        QAction* cancelAction = popup.addAction(SmallIcon("stop"), i18n("Cancel"));
        popup.insertSeparator(cancelAction);

        popup.exec(QCursor::pos());
    }

    m_droppedUrls.clear();
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

void DolphinMainWindow::slotViewModeChanged()
{
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
    updateGoActions();
    setCaption(url.fileName());
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(show);
}

void DolphinMainWindow::redo()
{
    UndoManager::instance().redo(this);
}

void DolphinMainWindow::undo()
{
    UndoManager::instance().undo(this);
}

void DolphinMainWindow::openNewMainWindow()
{
    DolphinApplication::app()->createMainWindow()->show();
}

void DolphinMainWindow::moveDroppedItems()
{
    moveUrls(m_droppedUrls, m_dropDestination);
}

void DolphinMainWindow::copyDroppedItems()
{
    copyUrls(m_droppedUrls, m_dropDestination);
}

void DolphinMainWindow::linkDroppedItems()
{
    KIO::Job* job = KIO::link(m_droppedUrls, m_dropDestination);
    addPendingUndoJob(job, DolphinCommand::Link, m_droppedUrls, m_dropDestination);
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();
    generalSettings->setFirstRun(false);

    settings.save();

    // TODO: I assume there will be a generic way in KDE 4 to store the docks
    // of the main window. In the meantime they are stored manually:
    QString filename = KStandardDirs::locateLocal("data", KGlobal::instance()->instanceName());
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
    config->setGroup("Primary view");
    config->writeEntry("Url", m_view[PrimaryIdx]->url().url());
    config->writeEntry("Editable Url", m_view[PrimaryIdx]->isUrlEditable());
    if (m_view[SecondaryIdx] != 0) {
        config->setGroup("Secondary view");
        config->writeEntry("Url", m_view[SecondaryIdx]->url().url());
        config->writeEntry("Editable Url", m_view[SecondaryIdx]->isUrlEditable());
    }
}

void DolphinMainWindow::readProperties(KConfig* config)
{
    config->setGroup("Primary view");
    m_view[PrimaryIdx]->setUrl(config->readEntry("Url"));
    m_view[PrimaryIdx]->setUrlEditable(config->readEntry("Editable Url", false));
    if (config->hasGroup("Secondary view")) {
        config->setGroup("Secondary view");
        if (m_view[SecondaryIdx] == 0) {
            toggleSplitView();
        }
        m_view[SecondaryIdx]->setUrl(config->readEntry("Url"));
        m_view[SecondaryIdx]->setUrlEditable(config->readEntry("Editable Url", false));
    }
    else if (m_view[SecondaryIdx] != 0) {
        toggleSplitView();
    }
}

void DolphinMainWindow::createFolder()
{
    // Parts of the following code have been taken
    // from the class KonqPopupMenu located in
    // libqonq/konq_popupmenu.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>,
    // Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>)

    clearStatusBar();

    DolphinStatusBar* statusBar = m_activeView->statusBar();
    const KUrl baseUrl(m_activeView->url());

    QString name(i18n("New Folder"));
    baseUrl.path(KUrl::AddTrailingSlash);


    if (baseUrl.isLocalFile() && QFileInfo(baseUrl.path(KUrl::AddTrailingSlash) + name).exists()) {
        name = KIO::RenameDialog::suggestName(baseUrl, i18n("New Folder"));
    }

    bool ok = false;
    name = KInputDialog::getText(i18n("New Folder"),
                                 i18n("Enter folder name:" ),
                                 name,
                                 &ok,
                                 this);

    if (!ok) {
        // the user has pressed 'Cancel'
        return;
    }

    assert(!name.isEmpty());

    KUrl url;
    if ((name[0] == '/') || (name[0] == '~')) {
        url.setPath(KShell::tildeExpand(name));
    }
    else {
        name = KIO::encodeFileName(name);
        url = baseUrl;
        url.addPath(name);
    }
    ok = KIO::NetAccess::mkdir(url, this);

    // TODO: provide message type hint
    if (ok) {
        statusBar->setMessage(i18n("Created folder %1.",url.path()),
                              DolphinStatusBar::OperationCompleted);

        DolphinCommand command(DolphinCommand::CreateFolder, KUrl::List(), url);
        UndoManager::instance().addCommand(command);
    }
    else {
        // Creating of the folder has been failed. Check whether the creating
        // has been failed because a folder with the same name exists...
        if (KIO::NetAccess::exists(url, true, this)) {
            statusBar->setMessage(i18n("A folder named %1 already exists.",url.path()),
                                  DolphinStatusBar::Error);
        }
        else {
            statusBar->setMessage(i18n("Creating of folder %1 failed.",url.path()),
                                  DolphinStatusBar::Error);
        }

    }
}

void DolphinMainWindow::createFile()
{
    // Parts of the following code have been taken
    // from the class KonqPopupMenu located in
    // libqonq/konq_popupmenu.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>,
    // Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>)

    clearStatusBar();

    // TODO: const Entry& entry = m_createFileTemplates[QString(sender->name())];
    // should be enough. Anyway: the implemantation of [] does a linear search internally too.
    KSortableList<CreateFileEntry, QString>::ConstIterator it = m_createFileTemplates.begin();
    KSortableList<CreateFileEntry, QString>::ConstIterator end = m_createFileTemplates.end();

    const QString senderName(sender()->objectName());
    bool found = false;
    CreateFileEntry entry;
    while (!found && (it != end)) {
        if ((*it).key() == senderName) {
            entry = (*it).value();
            found = true;
        }
        else {
            ++it;
        }
    }

    DolphinStatusBar* statusBar = m_activeView->statusBar();
    if (!found || !QFile::exists(entry.templatePath)) {
        statusBar->setMessage(i18n("Could not create file."), DolphinStatusBar::Error);
       return;
    }

    // Get the source path of the template which should be copied.
    // The source path is part of the Url entry of the desktop file.
    const int pos = entry.templatePath.lastIndexOf('/');
    QString sourcePath(entry.templatePath.left(pos + 1));
    sourcePath += KDesktopFile(entry.templatePath, true).readPathEntry("Url");

    QString name(i18n(entry.name.toAscii()));
    // Most entry names end with "..." (e. g. "HTML File..."), which is ok for
    // menus but no good choice for a new file name -> remove the dots...
    name.replace("...", QString::null);

    // add the file extension to the name
    name.append(sourcePath.right(sourcePath.length() - sourcePath.lastIndexOf('.')));

    // Check whether a file with the current name already exists. If yes suggest automatically
    // a unique file name (e. g. "HTML File" will be replaced by "HTML File_1").
    const KUrl viewUrl(m_activeView->url());
    const bool fileExists = viewUrl.isLocalFile() &&
                            QFileInfo(viewUrl.path(KUrl::AddTrailingSlash) + KIO::encodeFileName(name)).exists();
    if (fileExists) {
        name = KIO::RenameDialog::suggestName(viewUrl, name);
    }

    // let the user change the suggested file name
    bool ok = false;
    name = KInputDialog::getText(entry.name,
                                 entry.comment,
                                 name,
                                 &ok,
                                 this);
    if (!ok) {
        // the user has pressed 'Cancel'
        return;
    }

    // before copying the template to the destination path check whether a file
    // with the given name already exists
    const QString destPath(viewUrl.pathOrUrl() + "/" + KIO::encodeFileName(name));
    const KUrl destUrl(destPath);
    if (KIO::NetAccess::exists(destUrl, false, this)) {
        statusBar->setMessage(i18n("A file named %1 already exists.",name),
                              DolphinStatusBar::Error);
        return;
    }

    // copy the template to the destination path
    const KUrl sourceUrl(sourcePath);
    KIO::CopyJob* job = KIO::copyAs(sourceUrl, destUrl);
    job->setDefaultPermissions(true);
    if (KIO::NetAccess::synchronousRun(job, this)) {
        statusBar->setMessage(i18n("Created file %1.",name),
                              DolphinStatusBar::OperationCompleted);

        KUrl::List list;
        list.append(sourceUrl);
        DolphinCommand command(DolphinCommand::CreateFile, list, destUrl);
        UndoManager::instance().addCommand(command);

    }
    else {
        statusBar->setMessage(i18n("Creating of file %1 failed.",name),
                              DolphinStatusBar::Error);
    }
}

void DolphinMainWindow::rename()
{
    clearStatusBar();
    m_activeView->renameSelectedItems();
}

void DolphinMainWindow::moveToTrash()
{
    clearStatusBar();
    KUrl::List selectedUrls = m_activeView->selectedUrls();
    KIO::Job* job = KIO::trash(selectedUrls);
    addPendingUndoJob(job, DolphinCommand::Trash, selectedUrls, m_activeView->url());
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
                                                        QString::null,
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
        m_activeView->statusBar()->setMessage(job->errorString(),
                                              DolphinStatusBar::Error);
    }
}

void DolphinMainWindow::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        m_activeView->statusBar()->setMessage(i18n("Delete operation completed."),
                                               DolphinStatusBar::OperationCompleted);

        // TODO: In opposite to the 'Move to Trash' operation in the class KFileIconView
        // no rearranging of the item position is done when a file has been deleted.
        // This is bypassed by reloading the view, but it might be worth to investigate
        // deeper for the root of this issue.
        m_activeView->reload();
    }
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString& text)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::slotRedoAvailable(bool available)
{
    QAction* redoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Redo));
    if (redoAction != 0) {
        redoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotRedoTextChanged(const QString& text)
{
    QAction* redoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Redo));
    if (redoAction != 0) {
        redoAction->setText(text);
    }
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
            pasteAction->setText(i18n("Paste %1 Files").arg(count));
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
    m_activeView->slotShowFilterBar(show);
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
//    action->setChecked(action->setChecked);
    m_activeView->setUrlEditable(editOrBrowse);
}

void DolphinMainWindow::editLocation()
{
    KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("editable_location"));
    action->setChecked(true);
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

void DolphinMainWindow::addUndoOperation(KJob* job)
{
    if (job->error() != 0) {
        slotHandleJobError(job);
    }
    else {
        const int id = job->progressId();

        // set iterator to the executed command with the current id...
        Q3ValueList<UndoInfo>::Iterator it = m_pendingUndoJobs.begin();
        const Q3ValueList<UndoInfo>::Iterator end = m_pendingUndoJobs.end();
        bool found = false;
        while (!found && (it != end)) {
            if ((*it).id == id) {
                found = true;
            }
            else {
                ++it;
            }
        }

        if (found) {
            DolphinCommand command = (*it).command;
            if (command.type() == DolphinCommand::Trash) {
                // To be able to perform an undo for the 'Move to Trash' operation
                // all source Urls must be updated with the trash Url. E. g. when moving
                // a file "test.txt" and a second file "test.txt" to the trash,
                // then the filenames in the trash are "0-test.txt" and "1-test.txt".
                QMap<QString, QString> metaData;
                KIO::Job *kiojob = qobject_cast<KIO::Job*>( job );
                if ( kiojob )
                {
                    metaData = kiojob->metaData();
                }
                KUrl::List newSourceUrls;

                KUrl::List sourceUrls = command.source();
                KUrl::List::Iterator sourceIt = sourceUrls.begin();
                const KUrl::List::Iterator sourceEnd = sourceUrls.end();

                while (sourceIt != sourceEnd) {
                    QMap<QString, QString>::ConstIterator metaIt = metaData.find("trashUrl-" + (*sourceIt).path());
                    if (metaIt != metaData.end()) {
                        newSourceUrls.append(KUrl(metaIt.value()));
                    }
                    ++sourceIt;
                }
                command.setSource(newSourceUrls);
            }

            UndoManager::instance().addCommand(command);
            m_pendingUndoJobs.erase(it);

            DolphinStatusBar* statusBar = m_activeView->statusBar();
            switch (command.type()) {
                case DolphinCommand::Copy:
                    statusBar->setMessage(i18n("Copy operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                case DolphinCommand::Move:
                    statusBar->setMessage(i18n("Move operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                case DolphinCommand::Trash:
                    statusBar->setMessage(i18n("Move to trash operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                default:
                    break;
            }
        }
    }
}

void DolphinMainWindow::init()
{
    // Check whether Dolphin runs the first time. If yes then
    // a proper default window size is given at the end of DolphinMainWindow::init().
    GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    const bool firstRun = generalSettings->firstRun();

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

    const KUrl& homeUrl = root.first().url();
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

    setupCreateNewMenuActions();

    loadSettings();

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(640, 480);
    }
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
    QString filename = KStandardDirs::locateLocal("data", KGlobal::instance()->instanceName());
    filename.append("/panels_layout");
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
    KAction *action = new KAction(KIcon("window_new"),  i18n( "New &Window" ), actionCollection(), "new_window" );
    connect(action, SIGNAL(triggered()), this, SLOT(openNewMainWindow()));

    KAction* createFolder = new KAction(i18n("Folder..."), actionCollection(), "create_folder");
    createFolder->setIcon(KIcon("folder"));
    createFolder->setShortcut(Qt::Key_N);
    connect(createFolder, SIGNAL(triggered()), this, SLOT(createFolder()));

    KAction* rename = new KAction(i18n("Rename"), actionCollection(), "rename");
    rename->setShortcut(Qt::Key_F2);
    connect(rename, SIGNAL(triggered()), this, SLOT(rename()));

    KAction* moveToTrash = new KAction(i18n("Move to Trash"), actionCollection(), "move_to_trash");
    moveToTrash->setIcon(KIcon("edittrash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered()), this, SLOT(moveToTrash()));

    KAction* deleteAction = new KAction(i18n("Delete"), actionCollection(), "delete");
    deleteAction->setShortcut(Qt::ALT | Qt::Key_Delete);
    deleteAction->setIcon(KIcon("editdelete"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItems()));

    KAction* properties = new KAction(i18n("Propert&ies"), actionCollection(), "properties");
    properties->setShortcut(Qt::Key_Alt | Qt::Key_Return);
    connect(properties, SIGNAL(triggered()), this, SLOT(properties()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    UndoManager& undoManager = UndoManager::instance();
    KStandardAction::undo(this,
                     SLOT(undo()),
                     actionCollection());
    connect(&undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(&undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));

    KStandardAction::redo(this,
                     SLOT(redo()),
                     actionCollection());
    connect(&undoManager, SIGNAL(redoAvailable(bool)),
            this, SLOT(slotRedoAvailable(bool)));
    connect(&undoManager, SIGNAL(redoTextChanged(const QString&)),
            this, SLOT(slotRedoTextChanged(const QString&)));

    KStandardAction::cut(this, SLOT(cut()), actionCollection());
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KStandardAction::paste(this, SLOT(paste()), actionCollection());

    KAction* selectAll = new KAction(i18n("Select All"), actionCollection(), "select_all");
    selectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    KAction* invertSelection = new KAction(i18n("Invert Selection"), actionCollection(), "invert_selection");
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), this, SLOT(invertSelection()));

    // setup 'View' menu
    KStandardAction::zoomIn(this,
                       SLOT(zoomIn()),
                       actionCollection());

    KStandardAction::zoomOut(this,
                        SLOT(zoomOut()),
                        actionCollection());

    KToggleAction* iconsView = new KToggleAction(i18n("Icons"), actionCollection(), "icons");
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("view_icon"));
    connect(iconsView, SIGNAL(triggered()), this, SLOT(setIconsView()));

    KToggleAction* detailsView = new KToggleAction(i18n("Details"), actionCollection(), "details");
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("view_text"));
    connect(detailsView, SIGNAL(triggered()), this, SLOT(setDetailsView()));

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(iconsView);
    viewModeGroup->addAction(detailsView);

    KToggleAction* sortByName = new KToggleAction(i18n("By Name"), actionCollection(), "by_name");
    connect(sortByName, SIGNAL(triggered()), this, SLOT(sortByName()));

    KToggleAction* sortBySize = new KToggleAction(i18n("By Size"), actionCollection(), "by_size");
    connect(sortBySize, SIGNAL(triggered()), this, SLOT(sortBySize()));

    KToggleAction* sortByDate = new KToggleAction(i18n("By Date"), actionCollection(), "by_date");
    connect(sortByDate, SIGNAL(triggered()), this, SLOT(sortByDate()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->addAction(sortByName);
    sortGroup->addAction(sortBySize);
    sortGroup->addAction(sortByDate);

    KToggleAction* sortDescending = new KToggleAction(i18n("Descending"), actionCollection(), "descending");
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    KToggleAction* showPreview = new KToggleAction(i18n("Show Preview"), actionCollection(), "show_preview");
    connect(showPreview, SIGNAL(triggered()), this, SLOT(togglePreview()));

    KToggleAction* showHiddenFiles = new KToggleAction(i18n("Show Hidden Files"), actionCollection(), "show_hidden_files");
    //showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_      KDE4-TODO: what Qt-Key represents '.'?
    connect(showHiddenFiles, SIGNAL(triggered()), this, SLOT(toggleShowHiddenFiles()));

    KToggleAction* split = new KToggleAction(i18n("Split View"), actionCollection(), "split_view");
    split->setShortcut(Qt::Key_F10);
    split->setIcon(KIcon("view_left_right"));
    connect(split, SIGNAL(triggered()), this, SLOT(toggleSplitView()));

    KAction* reload = new KAction(actionCollection(), "reload");
    reload->setText(i18n("Reload"));
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(KIcon("reload"));
    connect(reload, SIGNAL(triggered()), this, SLOT(reloadView()));

    KAction* stop = new KAction(i18n("Stop"), actionCollection(), "stop");
    stop->setIcon(KIcon("stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    KToggleAction* showFullLocation = new KToggleAction(i18n("Show Full Location"), actionCollection(), "editable_location");
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KToggleAction* editLocation = new KToggleAction(i18n("Edit Location"), actionCollection(), "edit_location");
    editLocation->setShortcut(Qt::Key_F6);
    connect(editLocation, SIGNAL(triggered()), this, SLOT(editLocation()));

    KAction* adjustViewProps = new KAction(i18n("Adjust View Properties..."), actionCollection(), "view_properties");
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(adjustViewProperties()));

    // setup 'Go' menu
    KStandardAction::back(this, SLOT(goBack()), actionCollection());
    KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    KStandardAction::up(this, SLOT(goUp()), actionCollection());
    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    KAction* openTerminal = new KAction(i18n("Open Terminal"), actionCollection(), "open_terminal");
    openTerminal->setShortcut(Qt::Key_F4);
    openTerminal->setIcon(KIcon("konsole"));
    connect(openTerminal, SIGNAL(triggered()), this, SLOT(openTerminal()));

    KAction* findFile = new KAction(i18n("Find File..."), actionCollection(), "find_file");
    findFile->setShortcut(Qt::Key_F);
    findFile->setIcon(KIcon("filefind"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(findFile()));

    KToggleAction* showFilterBar = new KToggleAction(i18n("Show Filter Bar"), actionCollection(), "show_filter_bar");
    showFilterBar->setShortcut(Qt::Key_Slash);
    connect(showFilterBar, SIGNAL(triggered()), this, SLOT(showFilterBar()));

    KAction* compareFiles = new KAction(i18n("Compare Files"), actionCollection(), "compare_files");
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

    shortcutsDock->toggleViewAction()->setObjectName("show_bookmarks_panel");
    shortcutsDock->toggleViewAction()->setText(i18n("Show Bookmarks Panel"));
    actionCollection()->insert(shortcutsDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, shortcutsDock);

    QDockWidget* infoDock = new QDockWidget(i18n("Information"));
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    infoDock->setWidget(new InfoSidebarPage(this));

    infoDock->toggleViewAction()->setObjectName("show_info_panel");
    infoDock->toggleViewAction()->setText(i18n("Show Information Panel"));
    actionCollection()->insert(infoDock->toggleViewAction());

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
}

void DolphinMainWindow::setupCreateNewMenuActions()
{
    // Parts of the following code have been taken
    // from the class KNewMenu located in
    // libqonq/knewmenu.h of Konqueror.
    //  Copyright (C) 1998, 1999 David Faure <faure@kde.org>
    //                2003       Sven Leiber <s.leiber@web.de>

    QStringList files = actionCollection()->instance()->dirs()->findAllResources("templates");
    for (QStringList::Iterator it = files.begin() ; it != files.end(); ++it) {
        if ((*it)[0] != '.' ) {
            KSimpleConfig config(*it, true);
            config.setDesktopGroup();

            // tricky solution to ensure that TextFile is at the beginning
            // because this filetype is the most used (according kde-core discussion)
            const QString name(config.readEntry("Name"));
            QString key(name);

            const QString path(config.readPathEntry("Url"));
            if (!path.endsWith("emptydir")) {
                if (path.endsWith("TextFile.txt")) {
                    key = "1" + key;
                }
                else if (!KDesktopFile::isDesktopFile(path)) {
                    key = "2" + key;
                }
                else if (path.endsWith("Url.desktop")){
                    key = "3" + key;
                }
                else if (path.endsWith("Program.desktop")){
                    key = "4" + key;
                }
                else {
                    key = "5";
                }

                const QString icon(config.readEntry("Icon"));
                const QString comment(config.readEntry("Comment"));
                const QString type(config.readEntry("Type"));

                const QString filePath(*it);


                if (type == "Link") {
                    CreateFileEntry entry;
                    entry.name = name;
                    entry.icon = icon;
                    entry.comment = comment;
                    entry.templatePath = filePath;
                    m_createFileTemplates.insert(key, entry);
                }
            }
        }
    }
    m_createFileTemplates.sort();

    unplugActionList("create_actions");
    KSortableList<CreateFileEntry, QString>::ConstIterator it = m_createFileTemplates.begin();
    KSortableList<CreateFileEntry, QString>::ConstIterator end = m_createFileTemplates.end();
    /* KDE4-TODO: don't port this code; use KNewMenu instead
    while (it != end) {
        CreateFileEntry entry = (*it).value();
        KAction* action = new KAction(entry.name);
        action->setIcon(entry.icon);
        action->setName((*it).index());
        connect(action, SIGNAL(activated()),
                this, SLOT(createFile()));

        const QChar section = ((*it).index()[0]);
        switch (section) {
            case '1':
            case '2': {
                m_fileGroupActions.append(action);
                break;
            }

            case '3':
            case '4': {
                // TODO: not used yet. See documentation of DolphinMainWindow::linkGroupActions()
                // and DolphinMainWindow::linkToDeviceActions() in the header file for details.
                //m_linkGroupActions.append(action);
                break;
            }

            case '5': {
                // TODO: not used yet. See documentation of DolphinMainWindow::linkGroupActions()
                // and DolphinMainWindow::linkToDeviceActions() in the header file for details.
                //m_linkToDeviceActions.append(action);
                break;
            }
            default:
                break;
        }
        ++it;
    }

    plugActionList("create_file_group", m_fileGroupActions);
    //plugActionList("create_link_group", m_linkGroupActions);
    //plugActionList("link_to_device", m_linkToDeviceActions);*/
}

void DolphinMainWindow::updateHistory()
{
    int index = 0;
    const Q3ValueList<UrlNavigator::HistoryElem> list = m_activeView->urlHistory(index);

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
        //case DolphinView::PreviewsView:
        //    action = actionCollection()->action("previews");
        //    break;
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

    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(m_activeView->showHiddenFiles());

    KToggleAction* splitAction = static_cast<KToggleAction*>(actionCollection()->action("split_view"));
    splitAction->setChecked(m_view[SecondaryIdx] != 0);
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Up));
    const KUrl& currentUrl = m_activeView->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::copyUrls(const KUrl::List& source, const KUrl& dest)
{
    KIO::Job* job = KIO::copy(source, dest);
    addPendingUndoJob(job, DolphinCommand::Copy, source, dest);
}

void DolphinMainWindow::moveUrls(const KUrl::List& source, const KUrl& dest)
{
    KIO::Job* job = KIO::move(source, dest);
    addPendingUndoJob(job, DolphinCommand::Move, source, dest);
}

void DolphinMainWindow::addPendingUndoJob(KIO::Job* job,
                                          DolphinCommand::Type commandType,
                                          const KUrl::List& source,
                                          const KUrl& dest)
{
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(addUndoOperation(KJob*)));

    UndoInfo undoInfo;
    undoInfo.id = job->progressId();
    undoInfo.command = DolphinCommand(commandType, source, dest);
    m_pendingUndoJobs.append(undoInfo);
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

#include "dolphinmainwindow.moc"
