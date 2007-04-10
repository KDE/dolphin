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

#include <QApplication>
#include <QClipboard>
#include <QDropEvent>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QTimer>
#include <QScrollBar>

#include <kdirmodel.h>
#include <kfileitemdelegate.h>
#include <kfileplacesmodel.h>
#include <klocale.h>
#include <kiconeffect.h>
#include <kio/netaccess.h>
#include <kio/renamedialog.h>
#include <kio/previewjob.h>
#include <kmimetyperesolver.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <kurl.h>

#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphinstatusbar.h"
#include "dolphinmainwindow.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphincontextmenu.h"
#include "dolphinitemcategorizer.h"
#include "filterbar.h"
#include "renamedialog.h"
#include "kurlnavigator.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

DolphinView::DolphinView(DolphinMainWindow* mainWindow,
                         QWidget* parent,
                         const KUrl& url,
                         Mode mode,
                         bool showHiddenFiles) :
        QWidget(parent),
        m_showProgress(false),
        m_blockContentsMovedSignal(false),
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
        m_columnView(0),
        m_fileItemDelegate(0),
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

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));

    m_urlNavigator = new KUrlNavigator(DolphinSettings::instance().placesModel(), url, this);
    m_urlNavigator->setUrlEditable(DolphinSettings::instance().generalSettings()->editableUrl());
    m_urlNavigator->setHomeUrl(DolphinSettings::instance().generalSettings()->homeUrl());
    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(loadDirectory(const KUrl&)));
    connect(m_urlNavigator, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&)),
            this, SLOT(dropUrls(const KUrl::List&, const KUrl&)));
    connect(m_urlNavigator, SIGNAL(activated()),
            this, SLOT(requestActivation()));
    connect(this, SIGNAL(contentsMoved(int, int)),
            m_urlNavigator, SLOT(savePosition(int, int)));

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
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(updateCutItems()));
    connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));
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
    connect(m_controller, SIGNAL(urlsDropped(const KUrl::List&, const QModelIndex&, QWidget*)),
            this, SLOT(dropUrls(const KUrl::List&, const QModelIndex&, QWidget*)));
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

    m_controller->setShowPreview(show);

    emit showPreviewChanged();
    reload();
}

bool DolphinView::showPreview() const
{
    return m_controller->showPreview();
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

void DolphinView::setCategorizedSorting(bool categorized)
{
    if (!supportsCategorizedSorting() || (categorized == categorizedSorting())) {
        return;
    }

    Q_ASSERT(m_iconsView != 0);
    if (categorized) {
        Q_ASSERT(m_iconsView->itemCategorizer() == 0);
        m_iconsView->setItemCategorizer(new DolphinItemCategorizer());
    } else {
        KItemCategorizer* categorizer = m_iconsView->itemCategorizer();
        m_iconsView->setItemCategorizer(0);
        delete categorizer;
    }

    ViewProperties props(m_urlNavigator->url());
    props.setCategorizedSorting(categorized);
    props.save();

    emit categorizedSortingChanged();
}

bool DolphinView::categorizedSorting() const
{
    if (!supportsCategorizedSorting()) {
        return false;
    }

    Q_ASSERT(m_iconsView != 0);
    return m_iconsView->itemCategorizer() != 0;
}

bool DolphinView::supportsCategorizedSorting() const
{
    return m_iconsView != 0;
}

void DolphinView::renameSelectedItems()
{
    DolphinView* view = mainWindow()->activeView();
    const KUrl::List urls = selectedUrls();
    if (urls.count() > 1) {
        // More than one item has been selected for renaming. Open
        // a rename dialog and rename all items afterwards.
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(dialog.errorString(),
                                          DolphinStatusBar::Error);
        } else {
            // TODO: check how this can be integrated into KonqUndoManager/KonqOperations
            // as one operation instead of n rename operations like it is done now...
            Q_ASSERT(newName.contains('#'));

            // iterate through all selected items and rename them...
            const int replaceIndex = newName.indexOf('#');
            Q_ASSERT(replaceIndex >= 0);
            int index = 1;

            KUrl::List::const_iterator it = urls.begin();
            KUrl::List::const_iterator end = urls.end();
            while (it != end) {
                const KUrl& oldUrl = *it;
                QString number;
                number.setNum(index++);

                QString name(newName);
                name.replace(replaceIndex, 1, number);

                if (oldUrl.fileName() != name) {
                    KUrl newUrl = oldUrl;
                    newUrl.setFileName(name);
                    m_mainWindow->rename(oldUrl, newUrl);
                }
                ++it;
            }
        }
    } else {
        // Only one item has been selected for renaming. Use the custom
        // renaming mechanism from the views.
        Q_ASSERT(urls.count() == 1);

        // TODO: Think about using KFileItemDelegate as soon as it supports editing.
        // Currently the RenameDialog is used, but I'm not sure whether inline renaming
        // is a benefit for the user at all -> let's wait for some input first...
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(dialog.errorString(),
                                          DolphinStatusBar::Error);
        } else {
            const KUrl& oldUrl = urls.first();
            KUrl newUrl = oldUrl;
            newUrl.setFileName(newName);
            m_mainWindow->rename(oldUrl, newUrl);
        }
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
    m_controller->triggerZoomIn();
}

void DolphinView::zoomOut()
{
    m_controller->triggerZoomOut();
}

bool DolphinView::isZoomInPossible() const
{
    return m_controller->isZoomInPossible();
}

bool DolphinView::isZoomOutPossible() const
{
    return m_controller->isZoomOutPossible();
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

void DolphinView::setAdditionalInfo(KFileItemDelegate::AdditionalInformation info)
{
    ViewProperties props(m_urlNavigator->url());
    props.setAdditionalInfo(info);

    m_fileItemDelegate->setAdditionalInformation(info);

    emit additionalInfoChanged(info);
    reload();
}

KFileItemDelegate::AdditionalInformation DolphinView::additionalInfo() const
{
    return m_fileItemDelegate->additionalInformation();
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
    m_urlNavigator->setUrlEditable(editable);
}

bool DolphinView::hasSelection() const
{
    return itemView()->selectionModel()->hasSelection();
}

void DolphinView::clearSelection()
{
    itemView()->selectionModel()->clear();
}

KFileItemList DolphinView::selectedItems() const
{
    const QAbstractItemView* view = itemView();

    // Our view has a selection, we will map them back to the DirModel
    // and then fill the KFileItemList.
    Q_ASSERT((view != 0) && (view->selectionModel() != 0));

    const QItemSelection selection = m_proxyModel->mapSelectionToSource(view->selectionModel()->selection());
    KFileItemList itemList;

    const QModelIndexList indexList = selection.indexes();
    QModelIndexList::const_iterator end = indexList.end();
    for (QModelIndexList::const_iterator it = indexList.begin(); it != end; ++it) {
        Q_ASSERT((*it).isValid());

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
    } else {
        // no destination exists, hence just move the file to
        // do the renaming
        ok = KIO::NetAccess::file_move(source, dest);
    }

    const QString destFileName = dest.fileName();
    if (ok) {
        m_statusBar->setMessage(i18n("Renamed file '%1' to '%2'.", source.fileName(), destFileName),
                                DolphinStatusBar::OperationCompleted);

        KonqOperations::rename(this, source, destFileName);
    } else {
        m_statusBar->setMessage(i18n("Renaming of file '%1' to '%2' failed.", source.fileName(), destFileName),
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
    if (!isActive()) {
        requestActivation();
    }

    const ViewProperties props(url);

    const Mode mode = props.viewMode();
    bool changeMode = (m_mode != mode);
    if (changeMode && isColumnViewActive()) {
        // The column view is active. Only change the
        // mode if the current URL is no child of the column view.
        if (m_dirLister->url().isParentOf(url)) {
            changeMode = false;
        }
    }

    if (changeMode) {
        m_mode = mode;
        createView();
        emit modeChanged();
    }

    const bool showHiddenFiles = props.showHiddenFiles();
    if (showHiddenFiles != m_dirLister->showingDotFiles()) {
        m_dirLister->setShowingDotFiles(showHiddenFiles);
        emit showHiddenFilesChanged();
    }

    const bool categorized = props.categorizedSorting();
    if (categorized != categorizedSorting()) {
        if (supportsCategorizedSorting()) {
            Q_ASSERT(m_iconsView != 0);
            if (categorized) {
                Q_ASSERT(m_iconsView->itemCategorizer() == 0);
                m_iconsView->setItemCategorizer(new DolphinItemCategorizer());
            } else {
                KItemCategorizer* categorizer = m_iconsView->itemCategorizer();
                m_iconsView->setItemCategorizer(0);
                delete categorizer;
            }
        }
        emit categorizedSortingChanged();
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

    KFileItemDelegate::AdditionalInformation info = props.additionalInfo();
    if (info != m_fileItemDelegate->additionalInformation()) {
        m_fileItemDelegate->setAdditionalInformation(info);

        emit additionalInfoChanged(info);
    }

    const bool showPreview = props.showPreview();
    if (showPreview != m_controller->showPreview()) {
        m_controller->setShowPreview(showPreview);
        emit showPreviewChanged();
    }

    startDirLister(url);
    emit urlChanged(url);

    m_statusBar->clear();
}

void DolphinView::triggerItem(const QModelIndex& index)
{
    if (!isValidNameIndex(index)) {
        return;
    }

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

    // Prefer the local path over the URL. This assures that the
    // volume space information is correct. Assuming that the URL is media:/sda1,
    // and the local path is /windows/C: For the URL the space info is related
    // to the root partition (and hence wrong) and for the local path the space
    // info is related to the windows partition (-> correct).
    const QString localPath(item->localPath());
    KUrl url;
    if (localPath.isEmpty()) {
        url = item->url();
    } else {
        url = localPath;
    }

    if (item->isDir()) {
        setUrl(url);
    } else if (item->isFile()) {
        // allow to browse through ZIP and tar files
        KMimeType::Ptr mime = item->mimeTypePtr();
        if (mime->is("application/zip")) {
            url.setProtocol("zip");
            setUrl(url);
        } else if (mime->is("application/x-tar") ||
                   mime->is("application/x-tarz") ||
                   mime->is("application/x-bzip-compressed-tar") ||
                   mime->is("application/x-compressed-tar") ||
                   mime->is("application/x-tzo")) {
            url.setProtocol("tar");
            setUrl(url);
        } else {
            item->run();
        }
    } else {
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
        } else {
            ++m_fileCount;
        }
        ++it;
    }

    updateStatusBar();

    m_blockContentsMovedSignal = false;
    QTimer::singleShot(0, this, SLOT(restoreContentsPos()));
}

void DolphinView::generatePreviews(const KFileItemList& items)
{
    if (m_controller->showPreview()) {
        KIO::PreviewJob* job = KIO::filePreview(items, 128);
        connect(job, SIGNAL(gotPreview(const KFileItem*, const QPixmap&)),
                this, SLOT(showPreview(const KFileItem*, const QPixmap&)));
    }
}

void DolphinView::showPreview(const KFileItem* item, const QPixmap& pixmap)
{
    Q_ASSERT(item != 0);
    const QModelIndex idx = m_dirModel->indexForItem(*item);
    if (idx.isValid() && (idx.column() == 0)) {
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(*item)) {
            KIconEffect iconEffect;
            const QPixmap cutPixmap = iconEffect.apply(pixmap, K3Icon::Desktop, K3Icon::DisabledState);
            m_dirModel->setData(idx, QIcon(cutPixmap), Qt::DecorationRole);
        } else {
            m_dirModel->setData(idx, QIcon(pixmap), Qt::DecorationRole);
        }
    }
}

void DolphinView::restoreContentsPos()
{
    KUrl currentUrl = m_urlNavigator->url();
    if (!currentUrl.isEmpty()) {
        QAbstractItemView* view = itemView();
        // TODO: view->setCurrentItem(m_urlNavigator->currentFileName());
        QPoint pos = m_urlNavigator->savedPosition();
        view->horizontalScrollBar()->setValue(pos.x());
        view->verticalScrollBar()->setValue(pos.y());
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
    emit selectionChanged(DolphinView::selectedItems());
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
        } else {
            m_statusBar->setMessage(i18n("The location '%1' is invalid.", location),
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

    m_cutItemsCache.clear();
    m_blockContentsMovedSignal = true;
    m_dirLister->stop();

    bool openDir = true;
    bool keepOldDirs = isColumnViewActive();
    if (keepOldDirs) {
        if (reload) {
            keepOldDirs = false;

            const KUrl& dirListerUrl = m_dirLister->url();
            if (dirListerUrl.isValid()) {
                const KUrl::List dirs = m_dirLister->directories();
                KUrl url;
                foreach(url, dirs) {
                    m_dirLister->updateDirectory(url);
                }
                openDir = false;
            }
        } else if (m_dirLister->directories().contains(url)) {
            // The dir lister contains the directory already, so
            // KDirLister::openUrl() may not been invoked twice.
            m_dirLister->updateDirectory(url);
            openDir = false;
        } else {
            const KUrl& dirListerUrl = m_dirLister->url();
            if ((dirListerUrl == url) || !m_dirLister->url().isParentOf(url)) {
                // The current URL is not a child of the dir lister
                // URL. This may happen when e. g. a bookmark has been selected
                // and hence the view must be reset.
                keepOldDirs = false;
            }
        }
    }

    if (openDir) {
        m_dirLister->openUrl(url, keepOldDirs, reload);
    }
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
    while (it != end) {
        KFileItem* item = *it;
        if (item->isDir()) {
            ++folderCount;
        } else {
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
    Q_ASSERT(m_filterBar != 0);
    if (show) {
        m_filterBar->show();
    } else {
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

void DolphinView::changeSelection(const KFileItemList& selection)
{
    clearSelection();
    if (selection.isEmpty()) {
        return;
    }
    KUrl baseUrl = url();
    KUrl url;
    QItemSelection new_selection;
    foreach(KFileItem* item, selection) {
        url = item->url().upUrl();
        if (baseUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
            QModelIndex index = m_proxyModel->mapFromSource(m_dirModel->indexForItem(*item));
            new_selection.select(index, index);
        }
    }
    itemView()->selectionModel()->select(new_selection,
                                         QItemSelectionModel::ClearAndSelect
                                         | QItemSelectionModel::Current);
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
    m_proxyModel->setFilterRegExp(nameFilter);
#endif
}

void DolphinView::openContextMenu(const QPoint& pos)
{
    KFileItem* item = 0;

    const QModelIndex index = itemView()->indexAt(pos);
    if (isValidNameIndex(index)) {
        item = fileItem(index);
    }

    DolphinContextMenu contextMenu(m_mainWindow, item, url());
    contextMenu.open();
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const QModelIndex& index,
                           QWidget* source)
{
    KFileItem* directory = 0;
    if (isValidNameIndex(index)) {
        KFileItem* item = fileItem(index);
        Q_ASSERT(item != 0);
        if (item->isDir()) {
            // the URLs are dropped above a directory
            directory = item;
        }
    }

    if ((directory == 0) && (source == itemView())) {
        // The dropping is done into the same viewport where
        // the dragging has been started. Just ignore this...
        return;
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
    if (!m_blockContentsMovedSignal) {
        emit contentsMoved(contentsX(), contentsY());
    }
}

void DolphinView::updateActivationState()
{
    m_urlNavigator->setActive(isActive());
    if (isActive()) {
        emit urlChanged(url());
        emit selectionChanged(selectedItems());
    }
}

void DolphinView::updateCutItems()
{
    // restore the icons of all previously selected items to the
    // original state...
    QList<CutItem>::const_iterator it = m_cutItemsCache.begin();
    QList<CutItem>::const_iterator end = m_cutItemsCache.end();
    while (it != end) {
        const QModelIndex index = m_dirModel->indexForUrl((*it).url);
        if (index.isValid()) {
            m_dirModel->setData(index, QIcon((*it).pixmap), Qt::DecorationRole);
        }
        ++it;
    }
    m_cutItemsCache.clear();

    // ... and apply an item effect to all currently cut items
    applyCutItemEffect();
}

void DolphinView::createView()
{
    // delete current view
    QAbstractItemView* view = itemView();
    if (view != 0) {
        m_topLayout->removeWidget(view);
        view->close();
        if (view == m_iconsView) {
            KItemCategorizer* categorizer = m_iconsView->itemCategorizer();
            m_iconsView->setItemCategorizer(0);
            delete categorizer;
        }
        view->deleteLater();
        view = 0;
        m_iconsView = 0;
        m_detailsView = 0;
        m_columnView = 0;
        m_fileItemDelegate = 0;
    }

    Q_ASSERT(m_iconsView == 0);
    Q_ASSERT(m_detailsView == 0);
    Q_ASSERT(m_columnView == 0);

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

    case ColumnView:
        m_columnView = new DolphinColumnView(this, m_controller);
        view = m_columnView;
        break;
    }

    Q_ASSERT(view != 0);

    m_fileItemDelegate = new KFileItemDelegate(view);
    view->setItemDelegate(m_fileItemDelegate);

    view->setModel(m_proxyModel);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);

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
    if (m_detailsView != 0) {
        return m_detailsView;
    } else if (m_columnView != 0) {
        return m_columnView;
    }

    return m_iconsView;
}

bool DolphinView::isValidNameIndex(const QModelIndex& index) const
{
    return index.isValid() && (index.column() == KDirModel::Name);
}

bool DolphinView::isCutItem(const KFileItem& item) const
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List cutUrls = KUrl::List::fromMimeData(mimeData);

    const KUrl& itemUrl = item.url();
    KUrl::List::const_iterator it = cutUrls.begin();
    const KUrl::List::const_iterator end = cutUrls.end();
    while (it != end) {
        if (*it == itemUrl) {
            return true;
        }
        ++it;
    }

    return false;
}

void DolphinView::applyCutItemEffect()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if (!KonqMimeData::decodeIsCutSelection(mimeData)) {
        return;
    }

    KFileItemList items(m_dirLister->items());
    KFileItemList::const_iterator it = items.begin();
    const KFileItemList::const_iterator end = items.end();
    while (it != end) {
        KFileItem* item = *it;
        if (isCutItem(*item)) {
            const QModelIndex index = m_dirModel->indexForItem(*item);
            const KFileItem* item = m_dirModel->itemForIndex(index);
            const QVariant value = m_dirModel->data(index, Qt::DecorationRole);
            if ((value.type() == QVariant::Icon) && (item != 0)) {
                const QIcon icon(qvariant_cast<QIcon>(value));
                QPixmap pixmap = icon.pixmap(128, 128);

                // remember current pixmap for the item to be able
                // to restore it when other items get cut
                CutItem cutItem;
                cutItem.url = item->url();
                cutItem.pixmap = pixmap;
                m_cutItemsCache.append(cutItem);

                // apply icon effect to the cut item
                KIconEffect iconEffect;
                pixmap = iconEffect.apply(pixmap, K3Icon::Desktop, K3Icon::DisabledState);
                m_dirModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
        ++it;
    }
}

#include "dolphinview.moc"
