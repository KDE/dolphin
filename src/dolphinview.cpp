/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
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

#include "dolphinview.h"

#include <assert.h>

#include <QApplication>
#include <QDropEvent>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QVBoxLayout>

#include <kdirmodel.h>
#include <kfileitemdelegate.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kio/renamedialog.h>
#include <kmimetyperesolver.h>
#include <konq_operations.h>
#include <kurl.h>

#include "dolphincontroller.h"
#include "dolphinstatusbar.h"
#include "dolphinmainwindow.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphincontextmenu.h"
#include "filterbar.h"
#include "renamedialog.h"
#include "urlnavigator.h"
#include "viewproperties.h"

DolphinView::DolphinView(DolphinMainWindow* mainWindow,
                         QWidget* parent,
                         const KUrl& url,
                         Mode mode,
                         bool showHiddenFiles) :
    QWidget(parent),
    m_showProgress(false),
    m_mode(mode),
    m_iconSize(0),
    m_folderCount(0),
    m_fileCount(0),
    m_mainWindow(mainWindow),
    m_topLayout(0),
    m_urlNavigator(0),
    m_controller(0),
    m_iconsView(0),
    m_detailsView(0),
    m_filterBar(0),
    m_statusBar(0),
    m_dirModel(0),
    m_dirLister(0),
    m_proxyModel(0)
{
    hide();
    setFocusPolicy(Qt::StrongFocus);
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    connect(m_mainWindow, SIGNAL(activeViewChanged()),
            this, SLOT(updateActivationState()));

    m_urlNavigator = new UrlNavigator(url, this);
    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(loadDirectory(const KUrl&)));
    connect(m_urlNavigator, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&)),
            this, SLOT(dropUrls(const KUrl::List&, const KUrl&)));
    connect(m_urlNavigator, SIGNAL(activated()),
            this, SLOT(requestActivation()));
    connect(this, SIGNAL(contentsMoved(int, int)),
            m_urlNavigator, SLOT(storeContentsPosition(int, int)));

    m_statusBar = new DolphinStatusBar(this);

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setShowingDotFiles(showHiddenFiles);
    m_dirLister->setDelayedMimeTypes(true);

    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(percent(int)),
            this, SLOT(updateProgress(int)));
    connect(m_dirLister, SIGNAL(deleteItem(KFileItem*)),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(updateItemCount()));
    connect(m_dirLister, SIGNAL(infoMessage(const QString&)),
            this, SLOT(showInfoMessage(const QString&)));
    connect(m_dirLister, SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));

    m_dirModel = new KDirModel();
    m_dirModel->setDirLister(m_dirLister);
    m_dirModel->setDropsAllowed(KDirModel::DropOnDirectory);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dirModel);

    m_controller = new DolphinController(this);
    connect(m_controller, SIGNAL(requestContextMenu(const QPoint&)),
            this, SLOT(openContextMenu(const QPoint&)));
    connect(m_controller, SIGNAL(urlsDropped(const KUrl::List&, const QPoint&)),
            this, SLOT(dropUrls(const KUrl::List&, const QPoint&)));
    connect(m_controller, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(updateSorting(DolphinView::Sorting)));
    connect(m_controller, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(updateSortOrder(Qt::SortOrder)));
    connect(m_controller, SIGNAL(itemTriggered(const QModelIndex&)),
            this, SLOT(triggerItem(const QModelIndex&)));
    connect(m_controller, SIGNAL(selectionChanged()),
            this, SLOT(emitSelectionChangedSignal()));
    connect(m_controller, SIGNAL(activated()),
            this, SLOT(requestActivation()));

    createView();

    m_iconSize = K3Icon::SizeMedium;

    m_filterBar = new FilterBar(this);
    m_filterBar->hide();
    connect(m_filterBar, SIGNAL(filterChanged(const QString&)),
           this, SLOT(changeNameFilter(const QString&)));
    connect(m_filterBar, SIGNAL(closeRequest()),
            this, SLOT(closeFilterBar()));

    m_topLayout->addWidget(m_urlNavigator);
    m_topLayout->addWidget(itemView());
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);

    loadDirectory(m_urlNavigator->url());
}

DolphinView::~DolphinView()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void DolphinView::setUrl(const KUrl& url)
{
    m_urlNavigator->setUrl(url);
    m_controller->setUrl(url);
}

const KUrl& DolphinView::url() const
{
    return m_urlNavigator->url();
}

bool DolphinView::isActive() const
{
    return m_mainWindow->activeView() == this;
}

void DolphinView::setMode(Mode mode)
{
    if (mode == m_mode) {
        return; // the wished mode is already set
    }

    m_mode = mode;

    ViewProperties props(m_urlNavigator->url());
    props.setViewMode(m_mode);

    createView();
    startDirLister(m_urlNavigator->url());

    emit modeChanged();
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setShowPreview(bool show)
{
    ViewProperties props(m_urlNavigator->url());
    props.setShowPreview(show);

    // TODO: wait until previews are possible with KFileItemDelegate

    emit showPreviewChanged();
}

bool DolphinView::showPreview() const
{
    // TODO: wait until previews are possible with KFileItemDelegate
    return true;
}

void DolphinView::setShowHiddenFiles(bool show)
{
    if (m_dirLister->showingDotFiles() == show) {
        return;
    }

    ViewProperties props(m_urlNavigator->url());
    props.setShowHiddenFiles(show);
    props.save();

    m_dirLister->setShowingDotFiles(show);

    emit showHiddenFilesChanged();

    reload();
}

bool DolphinView::showHiddenFiles() const
{
    return m_dirLister->showingDotFiles();
}

void DolphinView::renameSelectedItems()
{
    const KUrl::List urls = selectedUrls();
    if (urls.count() > 1) {
        // More than one item has been selected for renaming. Open
        // a rename dialog and rename all items afterwards.
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        DolphinView* view = mainWindow()->activeView();
        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(i18n("The new item name is invalid."),
                                          DolphinStatusBar::Error);
        }
        else {
            // TODO: check how this can be integrated into KonqUndoManager/KonqOperations

            //UndoManager& undoMan = UndoManager::instance();
            //undoMan.beginMacro();

            assert(newName.contains('#'));

            const int urlsCount = urls.count();

            // iterate through all selected items and rename them...
            const int replaceIndex = newName.indexOf('#');
            assert(replaceIndex >= 0);
            for (int i = 0; i < urlsCount; ++i) {
                const KUrl& source = urls[i];
                QString number;
                number.setNum(i + 1);

                QString name(newName);
                name.replace(replaceIndex, 1, number);

                if (source.fileName() != name) {
                    KUrl dest(source.upUrl());
                    dest.addPath(name);

                    const bool destExists = KIO::NetAccess::exists(dest, false, view);
                    if (destExists) {
                        view->statusBar()->setMessage(i18n("Renaming failed (item '%1' already exists).",name),
                                                      DolphinStatusBar::Error);
                        break;
                    }
                    else if (KIO::NetAccess::file_move(source, dest)) {
                        // TODO: From the users point of view he executed one 'rename n files' operation,
                        // but internally we store it as n 'rename 1 file' operations for the undo mechanism.
                        //DolphinCommand command(DolphinCommand::Rename, source, dest);
                        //undoMan.addCommand(command);
                    }
                }
            }

            //undoMan.endMacro();
        }
    }
    else {
        // Only one item has been selected for renaming. Use the custom
        // renaming mechanism from the views.
        assert(urls.count() == 1);
        // TODO:
        /*if (m_mode == DetailsView) {
            Q3ListViewItem* item = m_iconsView->firstChild();
            while (item != 0) {
                if (item->isSelected()) {
                    m_iconsView->rename(item, DolphinDetailsView::NameColumn);
                    break;
                }
                item = item->nextSibling();
            }
        }
        else {
            KFileIconViewItem* item = static_cast<KFileIconViewItem*>(m_iconsView->firstItem());
            while (item != 0) {
                if (item->isSelected()) {
                    item->rename();
                    break;
                }
                item = static_cast<KFileIconViewItem*>(item->nextItem());
            }
        }*/
    }
}

void DolphinView::selectAll()
{
    selectAll(QItemSelectionModel::Select);
}

void DolphinView::invertSelection()
{
    selectAll(QItemSelectionModel::Toggle);
}

DolphinStatusBar* DolphinView::statusBar() const
{
    return m_statusBar;
}

int DolphinView::contentsX() const
{

    return itemView()->horizontalScrollBar()->value();
}

int DolphinView::contentsY() const
{
    return itemView()->verticalScrollBar()->value();
}

void DolphinView::refreshSettings()
{
    startDirLister(m_urlNavigator->url());
}

void DolphinView::emitRequestItemInfo(const KUrl& url)
{
    emit requestItemInfo(url);
}

bool DolphinView::isFilterBarVisible() const
{
  return m_filterBar->isVisible();
}

bool DolphinView::isUrlEditable() const
{
    return m_urlNavigator->isUrlEditable();
}

void DolphinView::zoomIn()
{
    //itemEffectsManager()->zoomIn();
}

void DolphinView::zoomOut()
{
    //itemEffectsManager()->zoomOut();
}

bool DolphinView::isZoomInPossible() const
{
    return false; //itemEffectsManager()->isZoomInPossible();
}

bool DolphinView::isZoomOutPossible() const
{
    return false; //itemEffectsManager()->isZoomOutPossible();
}

void DolphinView::setSorting(Sorting sorting)
{
    if (sorting != this->sorting()) {
        updateSorting(sorting);
    }
}

DolphinView::Sorting DolphinView::sorting() const
{
    return m_proxyModel->sorting();
}

void DolphinView::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() != order) {
        updateSortOrder(order);
    }
}

Qt::SortOrder DolphinView::sortOrder() const
{
    return m_proxyModel->sortOrder();
}

void DolphinView::goBack()
{
    m_urlNavigator->goBack();
}

void DolphinView::goForward()
{
    m_urlNavigator->goForward();
}

void DolphinView::goUp()
{
    m_urlNavigator->goUp();
}

void DolphinView::goHome()
{
    m_urlNavigator->goHome();
}

void DolphinView::setUrlEditable(bool editable)
{
    m_urlNavigator->editUrl(editable);
}

const QLinkedList<UrlNavigator::HistoryElem> DolphinView::urlHistory(int& index) const
{
    return m_urlNavigator->history(index);
}

bool DolphinView::hasSelection() const
{
    return itemView()->selectionModel()->hasSelection();
}

KFileItemList DolphinView::selectedItems() const
{
    const QAbstractItemView* view = itemView();

    // Our view has a selection, we will map them back to the DirModel
    // and then fill the KFileItemList.
    assert((view != 0) && (view->selectionModel() != 0));

    const QItemSelection selection = m_proxyModel->mapSelectionToSource(view->selectionModel()->selection());
    KFileItemList itemList;

    const QModelIndexList indexList = selection.indexes();
    QModelIndexList::const_iterator end = indexList.end();
    for (QModelIndexList::const_iterator it = indexList.begin(); it != end; ++it) {
        assert((*it).isValid());

        KFileItem* item = m_dirModel->itemForIndex(*it);
        if (item != 0) {
            itemList.append(item);
        }
    }

    return itemList;
}

KUrl::List DolphinView::selectedUrls() const
{
    KUrl::List urls;

    const KFileItemList list = selectedItems();
    KFileItemList::const_iterator it = list.begin();
    const KFileItemList::const_iterator end = list.end();
    while (it != end) {
        KFileItem* item = *it;
        urls.append(item->url());
        ++it;
    }

    return urls;
}

KFileItem* DolphinView::fileItem(const QModelIndex index) const
{
    const QModelIndex dirModelIndex = m_proxyModel->mapToSource(index);
    return m_dirModel->itemForIndex(dirModelIndex);
}

void DolphinView::rename(const KUrl& source, const QString& newName)
{
    bool ok = false;

    if (newName.isEmpty() || (source.fileName() == newName)) {
        return;
    }

    KUrl dest(source.upUrl());
    dest.addPath(newName);

    const bool destExists = KIO::NetAccess::exists(dest,
                                                   false,
                                                   mainWindow()->activeView());
    if (destExists) {
        // the destination already exists, hence ask the user
        // how to proceed...
        KIO::RenameDialog renameDialog(this,
                                       i18n("File Already Exists"),
                                       source.path(),
                                       dest.path(),
                                       KIO::M_OVERWRITE);
        switch (renameDialog.exec()) {
            case KIO::R_OVERWRITE:
                // the destination should be overwritten
                ok = KIO::NetAccess::file_move(source, dest, -1, true);
                break;

            case KIO::R_RENAME: {
                // a new name for the destination has been used
                KUrl newDest(renameDialog.newDestUrl());
                ok = KIO::NetAccess::file_move(source, newDest);
                break;
            }

            default:
                // the renaming operation has been canceled
                reload();
                return;
        }
    }
    else {
        // no destination exists, hence just move the file to
        // do the renaming
        ok = KIO::NetAccess::file_move(source, dest);
    }

    const QString destFileName = dest.fileName();
    if (ok) {
        m_statusBar->setMessage(i18n("Renamed file '%1' to '%2'.",source.fileName(), destFileName),
                                DolphinStatusBar::OperationCompleted);

        KonqOperations::rename(this, source, destFileName);
    }
    else {
        m_statusBar->setMessage(i18n("Renaming of file '%1' to '%2' failed.",source.fileName(), destFileName),
                                DolphinStatusBar::Error);
        reload();
    }
}

void DolphinView::reload()
{
    startDirLister(m_urlNavigator->url(), true);
}

void DolphinView::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    mainWindow()->setActiveView(this);
}

DolphinMainWindow* DolphinView::mainWindow() const
{
    return m_mainWindow;
}

void DolphinView::loadDirectory(const KUrl& url)
{
    const ViewProperties props(url);

    const Mode mode = props.viewMode();
    if (m_mode != mode) {
        m_mode = mode;
        createView();
        emit modeChanged();
    }

    const bool showHiddenFiles = props.showHiddenFiles();
    if (showHiddenFiles != m_dirLister->showingDotFiles()) {
        m_dirLister->setShowingDotFiles(showHiddenFiles);
        emit showHiddenFilesChanged();
    }

    const DolphinView::Sorting sorting = props.sorting();
    if (sorting != m_proxyModel->sorting()) {
        m_proxyModel->setSorting(sorting);
        emit sortingChanged(sorting);
    }

    const Qt::SortOrder sortOrder = props.sortOrder();
    if (sortOrder != m_proxyModel->sortOrder()) {
        m_proxyModel->setSortOrder(sortOrder);
        emit sortOrderChanged(sortOrder);
    }

    // TODO: handle previews (props.showPreview())

    startDirLister(url);
    emit urlChanged(url);

    m_statusBar->clear();
}

void DolphinView::triggerItem(const QModelIndex& index)
{
    const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    if ((modifier & Qt::ShiftModifier) || (modifier & Qt::ControlModifier)) {
        // items are selected by the user, hence don't trigger the
        // item specified by 'index'
        return;
    }

    KFileItem* item = m_dirModel->itemForIndex(m_proxyModel->mapToSource(index));
    if (item == 0) {
        return;
    }

    if (item->isDir()) {
        // Prefer the local path over the URL. This assures that the
        // volume space information is correct. Assuming that the URL is media:/sda1,
        // and the local path is /windows/C: For the URL the space info is related
        // to the root partition (and hence wrong) and for the local path the space
        // info is related to the windows partition (-> correct).
        const QString localPath(item->localPath());
        if (localPath.isEmpty()) {
            setUrl(item->url());
        }
        else {
            setUrl(KUrl(localPath));
        }
    }
    else {
        item->run();
    }
}

void DolphinView::updateProgress(int percent)
{
    if (m_showProgress) {
        m_statusBar->setProgress(percent);
    }
}

void DolphinView::updateItemCount()
{
    if (m_showProgress) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
        m_showProgress = false;
    }

    KFileItemList items(m_dirLister->items());
    KFileItemList::const_iterator it = items.begin();
    const KFileItemList::const_iterator end = items.end();

    m_fileCount = 0;
    m_folderCount = 0;

    while (it != end) {
        KFileItem* item = *it;
        if (item->isDir()) {
            ++m_folderCount;
        }
        else {
            ++m_fileCount;
        }
        ++it;
    }

    updateStatusBar();

    QTimer::singleShot(0, this, SLOT(restoreContentsPos()));
}

void DolphinView::restoreContentsPos()
{
    int index = 0;
    const QLinkedList<UrlNavigator::HistoryElem> history = urlHistory(index);
    if (!history.isEmpty()) {
        QAbstractItemView* view = itemView();
        // TODO: view->setCurrentItem(history[index].currentFileName());

        QLinkedList<UrlNavigator::HistoryElem>::const_iterator it = history.begin();
        it += index;
        view->horizontalScrollBar()->setValue((*it).contentsX());
        view->verticalScrollBar()->setValue((*it).contentsY());
    }
}

void DolphinView::showInfoMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Information);
}

void DolphinView::showErrorMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Error);
}

void DolphinView::emitSelectionChangedSignal()
{
    emit selectionChanged();
}

void DolphinView::closeFilterBar()
{
    m_filterBar->hide();
    emit showFilterBarChanged(false);
}

void DolphinView::startDirLister(const KUrl& url, bool reload)
{
    if (!url.isValid()) {
        const QString location(url.pathOrUrl());
        if (location.isEmpty()) {
            m_statusBar->setMessage(i18n("The location is empty."), DolphinStatusBar::Error);
        }
        else {
            m_statusBar->setMessage(i18n("The location '%1' is invalid.",location),
                                    DolphinStatusBar::Error);
        }
        return;
    }

    // Only show the directory loading progress if the status bar does
    // not contain another progress information. This means that
    // the directory loading progress information has the lowest priority.
    const QString progressText(m_statusBar->progressText());
    m_showProgress = progressText.isEmpty() ||
                     (progressText == i18n("Loading directory..."));
    if (m_showProgress) {
        m_statusBar->setProgressText(i18n("Loading directory..."));
        m_statusBar->setProgress(0);
    }

    m_dirLister->stop();
    m_dirLister->openUrl(url, false, reload);
}

QString DolphinView::defaultStatusBarText() const
{
    return KIO::itemsSummaryString(m_fileCount + m_folderCount,
                                   m_fileCount,
                                   m_folderCount,
                                   0, false);
}

QString DolphinView::selectionStatusBarText() const
{
    QString text;
    const KFileItemList list = selectedItems();
    if (list.isEmpty()) {
        // when an item is triggered, it is temporary selected but selectedItems()
        // will return an empty list
        return QString();
    }

    int fileCount = 0;
    int folderCount = 0;
    KIO::filesize_t byteSize = 0;
    KFileItemList::const_iterator it = list.begin();
    const KFileItemList::const_iterator end = list.end();
    while (it != end){
        KFileItem* item = *it;
        if (item->isDir()) {
            ++folderCount;
        }
        else {
            ++fileCount;
            byteSize += item->size();
        }
        ++it;
    }

    if (folderCount > 0) {
        text = i18np("1 Folder selected", "%1 Folders selected", folderCount);
        if (fileCount > 0) {
            text += ", ";
        }
    }

    if (fileCount > 0) {
        const QString sizeText(KIO::convertSize(byteSize));
        text += i18np("1 File selected (%2)", "%1 Files selected (%2)", fileCount, sizeText);
    }

    return text;
}

void DolphinView::showFilterBar(bool show)
{
    assert(m_filterBar != 0);
    if (show) {
        m_filterBar->show();
    }
    else {
        m_filterBar->hide();
    }
}

void DolphinView::updateStatusBar()
{
    // As the item count information is less important
    // in comparison with other messages, it should only
    // be shown if:
    // - the status bar is empty or
    // - shows already the item count information or
    // - shows only a not very important information
    // - if any progress is given don't show the item count info at all
    const QString msg(m_statusBar->message());
    const bool updateStatusBarMsg = (msg.isEmpty() ||
                                     (msg == m_statusBar->defaultText()) ||
                                     (m_statusBar->type() == DolphinStatusBar::Information)) &&
                                    (m_statusBar->progress() == 100);

    const QString text(hasSelection() ? selectionStatusBarText() : defaultStatusBarText());
    m_statusBar->setDefaultText(text);

    if (updateStatusBarMsg) {
        m_statusBar->setMessage(text, DolphinStatusBar::Default);
    }
}

void DolphinView::requestActivation()
{
    m_mainWindow->setActiveView(this);
}

void DolphinView::changeNameFilter(const QString& nameFilter)
{
    // The name filter of KDirLister does a 'hard' filtering, which
    // means that only the items are shown where the names match
    // exactly the filter. This is non-transparent for the user, which
    // just wants to have a 'soft' filtering: does the name contain
    // the filter string?
    QString adjustedFilter(nameFilter);
    adjustedFilter.insert(0, '*');
    adjustedFilter.append('*');

    // Use the ProxyModel to filter:
    // This code is #ifdefed as setNameFilter behaves
    // slightly different than the QSortFilterProxyModel
    // as it will not remove directories. I will ask
    // our beloved usability experts for input
    // -- z.
#if 0
    m_dirLister->setNameFilter(adjustedFilter);
    m_dirLister->emitChanges();
#else
    m_proxyModel->setFilterRegExp( nameFilter );
#endif
}

void DolphinView::openContextMenu(const QPoint& pos)
{
    KFileItem* item = 0;

    const QModelIndex index = itemView()->indexAt(pos);
    if (index.isValid()) {
        item = fileItem(index);
    }

    DolphinContextMenu contextMenu(this, item);
    contextMenu.open();
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const QPoint& pos)
{
    KFileItem* directory = 0;
    const QModelIndex index = itemView()->indexAt(pos);
    if (index.isValid()) {
        KFileItem* item = fileItem(index);
        assert(item != 0);
        if (item->isDir()) {
            // the URLs are dropped above a directory
            directory = item;
        }
    }

    const KUrl& destination = (directory == 0) ? url() :
                                                 directory->url();
    dropUrls(urls, destination);
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const KUrl& destination)
{
    m_mainWindow->dropUrls(urls, destination);
}


void DolphinView::updateSorting(DolphinView::Sorting sorting)
{
    ViewProperties props(url());
    props.setSorting(sorting);

    m_proxyModel->setSorting(sorting);

    emit sortingChanged(sorting);
}

void DolphinView::updateSortOrder(Qt::SortOrder order)
{
    ViewProperties props(url());
    props.setSortOrder(order);

    m_proxyModel->setSortOrder(order);

    emit sortOrderChanged(order);
}

void DolphinView::emitContentsMoved()
{
    emit contentsMoved(contentsX(), contentsY());
}

void DolphinView::updateActivationState()
{
    m_urlNavigator->setActive(isActive());
}

void DolphinView::createView()
{
    // delete current view
    QAbstractItemView* view = itemView();
    if (view != 0) {
        m_topLayout->removeWidget(view);
        view->close();
        view->deleteLater();
        m_iconsView = 0;
        m_detailsView = 0;
    }

    assert(m_iconsView == 0);
    assert(m_detailsView == 0);

    // ... and recreate it representing the current mode
    switch (m_mode) {
        case IconsView:
            m_iconsView = new DolphinIconsView(this, m_controller);
            view = m_iconsView;
            break;

        case DetailsView:
            m_detailsView = new DolphinDetailsView(this, m_controller);
            view = m_detailsView;
            break;
    }

    view->setModel(m_proxyModel);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    KFileItemDelegate* delegate = new KFileItemDelegate(this);
    delegate->setAdditionalInformation(KFileItemDelegate::FriendlyMimeType);
    view->setItemDelegate(delegate);

    new KMimeTypeResolver(view, m_dirModel);
    m_topLayout->insertWidget(1, view);

    connect(view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            m_controller, SLOT(indicateSelectionChange()));
    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
}

void DolphinView::selectAll(QItemSelectionModel::SelectionFlags flags)
{
    QItemSelectionModel* selectionModel = itemView()->selectionModel();
    const QAbstractItemModel* itemModel = selectionModel->model();

    const QModelIndex topLeft = itemModel->index(0, 0);
    const QModelIndex bottomRight = itemModel->index(itemModel->rowCount() - 1,
                                                     itemModel->columnCount() - 1);

    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, flags);
}

QAbstractItemView* DolphinView::itemView() const
{
    assert((m_iconsView == 0) || (m_detailsView == 0));
    if (m_detailsView != 0) {
        return m_detailsView;
    }
    return m_iconsView;
}

#include "dolphinview.moc"
