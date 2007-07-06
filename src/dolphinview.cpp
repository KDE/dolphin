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
#include <QKeyEvent>
#include <QItemSelection>
#include <QBoxLayout>
#include <QTimer>
#include <QScrollBar>

#include <kcolorscheme.h>
#include <kdirmodel.h>
#include <kdirlister.h>
#include <kfileitemdelegate.h>
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
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphinitemcategorizer.h"
#include "renamedialog.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

DolphinView::DolphinView(QWidget* parent,
                         const KUrl& url,
                         KDirLister* dirLister,
                         KDirModel* dirModel,
                         DolphinSortFilterProxyModel* proxyModel) :
    QWidget(parent),
    m_active(true),
    m_loadingDirectory(false),
    m_initializeColumnView(false),
    m_mode(DolphinView::IconsView),
    m_topLayout(0),
    m_controller(0),
    m_iconsView(0),
    m_detailsView(0),
    m_columnView(0),
    m_fileItemDelegate(0),
    m_dirModel(dirModel),
    m_dirLister(dirLister),
    m_proxyModel(proxyModel)
{
    setFocusPolicy(Qt::StrongFocus);
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));

    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(updateCutItems()));
    connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));

    m_controller = new DolphinController(this);
    m_controller->setUrl(url);
    connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
            this, SIGNAL(urlChanged(const KUrl&)));
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
    connect(m_controller, SIGNAL(activated()),
            this, SLOT(activate()));
    connect(m_controller, SIGNAL(itemEntered(const QModelIndex&)),
            this, SLOT(showHoverInformation(const QModelIndex&)));
    connect(m_controller, SIGNAL(viewportEntered()),
            this, SLOT(clearHoverInformation()));

    applyViewProperties(url);
    m_topLayout->addWidget(itemView());
}

DolphinView::~DolphinView()
{
}

const KUrl& DolphinView::url() const
{
    return m_controller->url();
}

KUrl DolphinView::rootUrl() const
{
    return isColumnViewActive() ? m_dirLister->url() : url();
}

void DolphinView::setActive(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    QColor color = KColorScheme(KColorScheme::View).background();
    if (active) {
        emit urlChanged(url());
        emit selectionChanged(selectedItems());
    } else {
        color.setAlpha(0);
    }

    QWidget* viewport = itemView()->viewport();
    QPalette palette;
    palette.setColor(viewport->backgroundRole(), color);
    viewport->setPalette(palette);

    update();

    if (active) {
        emit activated();
    }
}

bool DolphinView::isActive() const
{
    return m_active;
}

void DolphinView::setMode(Mode mode)
{
    if (mode == m_mode) {
        return; // the wished mode is already set
    }

    m_mode = mode;

    if (isColumnViewActive()) {
        // When changing the mode in the column view, it makes sense
        // to go back to the root URL of the column view automatically.
        // Otherwise there it would not be possible to turn off the column view
        // without focusing the first column.
        setUrl(m_dirLister->url());
        m_controller->setUrl(m_dirLister->url());
    }

    ViewProperties props(url());
    props.setViewMode(m_mode);

    createView();
    startDirLister(url());

    emit modeChanged();
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setShowPreview(bool show)
{
    ViewProperties props(url());
    props.setShowPreview(show);

    m_controller->setShowPreview(show);
    emit showPreviewChanged();

    startDirLister(url(), true);
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

    ViewProperties props(url());
    props.setShowHiddenFiles(show);

    m_dirLister->setShowingDotFiles(show);
    emit showHiddenFilesChanged();

    startDirLister(url(), true);
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

    ViewProperties props(url());
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

void DolphinView::selectAll()
{
    selectAll(QItemSelectionModel::Select);
}

void DolphinView::invertSelection()
{
    selectAll(QItemSelectionModel::Toggle);
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

void DolphinView::setContentsPosition(int x, int y)
{
    QAbstractItemView* view = itemView();
    view->horizontalScrollBar()->setValue(x);
    view->verticalScrollBar()->setValue(y);

    m_loadingDirectory = false;
}

QPoint DolphinView::contentsPosition() const
{
    const int x = itemView()->horizontalScrollBar()->value();
    const int y = itemView()->verticalScrollBar()->value();
    return QPoint(x, y);
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
    ViewProperties props(url());
    props.setAdditionalInfo(info);

    m_controller->setShowAdditionalInfo(info != KFileItemDelegate::NoInformation);
    m_fileItemDelegate->setAdditionalInformation(info);

    emit additionalInfoChanged(info);
    startDirLister(url(), true);
}

KFileItemDelegate::AdditionalInformation DolphinView::additionalInfo() const
{
    return m_fileItemDelegate->additionalInformation();
}

void DolphinView::reload()
{
    setUrl(url());
    startDirLister(url(), true);
}

void DolphinView::refresh()
{
    createView();
    applyViewProperties(m_controller->url());
    reload();
}

void DolphinView::setUrl(const KUrl& url)
{
    if (m_controller->url() == url) {
        return;
    }

    m_controller->setUrl(url);

    applyViewProperties(url);

    startDirLister(url);
    emit urlChanged(url);
}

void DolphinView::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    setActive(true);;
}
void DolphinView::activate()
{
    setActive(true);
}

void DolphinView::triggerItem(const QModelIndex& index)
{
    if (!isValidNameIndex(index)) {
        clearSelection();
        showHoverInformation(index);
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

    // The stuff below should be moved to ViewContainer and be just a signal?

    // Prefer the local path over the URL.
    bool isLocal;
    KUrl url = item->mostLocalUrl(isLocal);

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

void DolphinView::generatePreviews(const KFileItemList& items)
{
    if (m_controller->showPreview()) {

        // Must turn QList<KFileItem *> to QList<KFileItem>...
        QList<KFileItem> itemsToPreview;
        foreach( KFileItem* it, items )
            itemsToPreview.append( *it );

        KIO::PreviewJob* job = KIO::filePreview(itemsToPreview, 128);
        connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
    }
}

void DolphinView::showPreview(const KFileItem& item, const QPixmap& pixmap)
{
    Q_ASSERT(!item.isNull());
    if (item.url().directory() != m_dirLister->url().path()) {
        // the preview job is still working on items of an older URL, hence
        // the item is not part of the directory model anymore
        return;
    }

    const QModelIndex idx = m_dirModel->indexForItem(item);
    if (idx.isValid() && (idx.column() == 0)) {
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            KIconEffect iconEffect;
            const QPixmap cutPixmap = iconEffect.apply(pixmap, K3Icon::Desktop, K3Icon::DisabledState);
            m_dirModel->setData(idx, QIcon(cutPixmap), Qt::DecorationRole);
        } else {
            m_dirModel->setData(idx, QIcon(pixmap), Qt::DecorationRole);
        }
    }
}

void DolphinView::emitSelectionChangedSignal()
{
    emit selectionChanged(DolphinView::selectedItems());
}

void DolphinView::startDirLister(const KUrl& url, bool reload)
{
    if (!url.isValid()) {
        const QString location(url.pathOrUrl());
        if (location.isEmpty()) {
            emit errorMessage(i18n("The location is empty."));
        } else {
            emit errorMessage(i18n("The location '%1' is invalid.", location));
        }
        return;
    }

    m_cutItemsCache.clear();
    m_loadingDirectory = true;

    m_dirLister->stop();

    bool openDir = true;
    bool keepOldDirs = isColumnViewActive() && !m_initializeColumnView;
    m_initializeColumnView = false;

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

void DolphinView::applyViewProperties(const KUrl& url)
{
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

        if (m_mode == ColumnView) {
            // The mode has been changed to the Column View. When starting the dir
            // lister with DolphinView::startDirLister() it is important to give a
            // hint that the dir lister may not keep the current directory
            // although this is the default for showing a hierarchy.
            m_initializeColumnView = true;
        }
    }
    if (itemView() == 0) {
        createView();
    }
    Q_ASSERT(itemView() != 0);
    Q_ASSERT(m_fileItemDelegate != 0);

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
        m_controller->setShowAdditionalInfo(info != KFileItemDelegate::NoInformation);
        m_fileItemDelegate->setAdditionalInformation(info);
        emit additionalInfoChanged(info);
    }

    const bool showPreview = props.showPreview();
    if (showPreview != m_controller->showPreview()) {
        m_controller->setShowPreview(showPreview);
        emit showPreviewChanged();
    }
}

void DolphinView::changeSelection(const KFileItemList& selection)
{
    clearSelection();
    if (selection.isEmpty()) {
        return;
    }
    const KUrl& baseUrl = url();
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

void DolphinView::openContextMenu(const QPoint& pos)
{
    KFileItem* item = 0;

    const QModelIndex index = itemView()->indexAt(pos);
    if (isValidNameIndex(index)) {
        item = fileItem(index);
    }

    emit requestContextMenu(item, url());
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

    const KUrl& destination = (directory == 0) ?
                              url() : directory->url();
    dropUrls(urls, destination);
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const KUrl& destination)
{
    emit urlsDropped(urls, destination);
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
    // only emit the contents moved signal if:
    // - no directory loading is ongoing (this would reset the contents position
    //   always to (0, 0))
    // - if the Column View is active: the column view does an automatic
    //   positioning during the loading operation, which must be remembered
    if (!m_loadingDirectory || isColumnViewActive()) {
        const QPoint pos(contentsPosition());
        emit contentsMoved(pos.x(), pos.y());
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

void DolphinView::showHoverInformation(const QModelIndex& index)
{
    if (hasSelection()) {
        return;
    }

    const KFileItem* item = fileItem(index);
    if (item != 0) {
        emit requestItemInfo(*item);
    }
}

void DolphinView::clearHoverInformation()
{
    emit requestItemInfo(KFileItem());
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
            this, SLOT(emitSelectionChangedSignal()));
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
