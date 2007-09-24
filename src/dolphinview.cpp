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

#include "dolphinmodel.h"
#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "renamedialog.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"
#include "dolphincategorydrawer.h"

DolphinView::DolphinView(QWidget* parent,
                         const KUrl& url,
                         KDirLister* dirLister,
                         DolphinModel* dolphinModel,
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
    m_dolphinModel(dolphinModel),
    m_dirLister(dirLister),
    m_proxyModel(proxyModel),
    m_rootUrl(url)
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
    connect(m_dirLister, SIGNAL(newItems(const QList<KFileItem>&)),
            this, SLOT(generatePreviews(const QList<KFileItem>&)));

    m_controller = new DolphinController(this);
    m_controller->setUrl(url);
    connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
            this, SIGNAL(urlChanged(const KUrl&)));
    connect(m_controller, SIGNAL(requestContextMenu(const QPoint&)),
            this, SLOT(openContextMenu(const QPoint&)));
    connect(m_controller, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&, const QModelIndex&, QWidget*)),
            this, SLOT(dropUrls(const KUrl::List&, const KUrl&, const QModelIndex&, QWidget*)));
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

void DolphinView::setRootUrl(const KUrl& url)
{
    m_rootUrl = url;
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

    updateViewportColor();
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

    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setViewMode(m_mode);

    createView();
    startDirLister(viewPropsUrl);

    emit modeChanged();
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setShowPreview(bool show)
{
    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setShowPreview(show);

    m_controller->setShowPreview(show);
    emit showPreviewChanged();

    startDirLister(viewPropsUrl, true);
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

    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setShowHiddenFiles(show);

    m_dirLister->setShowingDotFiles(show);
    emit showHiddenFilesChanged();

    startDirLister(viewPropsUrl, true);
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

    ViewProperties props(viewPropertiesUrl());
    props.setCategorizedSorting(categorized);
    props.save();

    m_proxyModel->setCategorizedModel(categorized);
    m_proxyModel->sort(m_proxyModel->sortColumn(), m_proxyModel->sortOrder());

    emit categorizedSortingChanged();
}

bool DolphinView::categorizedSorting() const
{
    return m_proxyModel->isCategorizedModel();
}

bool DolphinView::supportsCategorizedSorting() const
{
    return m_iconsView != 0;
}

void DolphinView::selectAll()
{
    itemView()->selectAll();
}

void DolphinView::invertSelection()
{
    QItemSelectionModel* selectionModel = itemView()->selectionModel();
    const QAbstractItemModel* itemModel = selectionModel->model();

    const QModelIndex topLeft = itemModel->index(0, 0);
    const QModelIndex bottomRight = itemModel->index(itemModel->rowCount() - 1,
                                                     itemModel->columnCount() - 1);

    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Toggle);
}

bool DolphinView::hasSelection() const
{
    return itemView()->selectionModel()->hasSelection();
}

void DolphinView::clearSelection()
{
    itemView()->selectionModel()->clear();
}

QList<KFileItem> DolphinView::selectedItems() const
{
    const QAbstractItemView* view = itemView();

    // Our view has a selection, we will map them back to the DolphinModel
    // and then fill the KFileItemList.
    Q_ASSERT((view != 0) && (view->selectionModel() != 0));

    const QItemSelection selection = m_proxyModel->mapSelectionToSource(view->selectionModel()->selection());
    QList<KFileItem> itemList;

    const QModelIndexList indexList = selection.indexes();
    foreach (QModelIndex index, indexList) {
        KFileItem item = m_dolphinModel->itemForIndex(index);
        if (!item.isNull()) {
            itemList.append(item);
        }
    }

    return itemList;
}

KUrl::List DolphinView::selectedUrls() const
{
    KUrl::List urls;
    const QList<KFileItem> list = selectedItems();
    for ( QList<KFileItem>::const_iterator it = list.begin(), end = list.end();
          it != end; ++it ) {
        urls.append((*it).url());
    }
    return urls;
}

KFileItem DolphinView::fileItem(const QModelIndex& index) const
{
    const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
    return m_dolphinModel->itemForIndex(dolphinModelIndex);
}

void DolphinView::setContentsPosition(int x, int y)
{
    QAbstractItemView* view = itemView();

    // the ColumnView takes care itself for the horizontal scrolling
    if (!isColumnViewActive()) {
        view->horizontalScrollBar()->setValue(x);
    }
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
    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setAdditionalInfo(info);

    m_controller->setShowAdditionalInfo(info != KFileItemDelegate::NoInformation);
    m_fileItemDelegate->setAdditionalInformation(info);

    emit additionalInfoChanged(info);
    startDirLister(viewPropsUrl, true);
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
    updateViewportColor();
}

void DolphinView::setUrl(const KUrl& url)
{
    if (m_controller->url() == url) {
        return;
    }

    const KUrl oldRootUrl = rootUrl();
    m_controller->setUrl(url); // emits urlChanged, which we forward

    bool useUrlProperties = true;
    const bool restoreColumnView = !isColumnViewActive()
                                   && m_rootUrl.isParentOf(url)
                                   && (m_rootUrl != url);
    if (restoreColumnView) {
        applyViewProperties(m_rootUrl);
        if (itemView() == m_columnView) {
            startDirLister(m_rootUrl);
            m_columnView->showColumn(url);
            useUrlProperties = false;
        }
    }

    if (useUrlProperties) {
        applyViewProperties(url);
        startDirLister(url);
    }

    itemView()->setFocus();

    const KUrl newRootUrl = rootUrl();
    if (newRootUrl != oldRootUrl) {
        emit rootUrlChanged(newRootUrl);
    }
}

void DolphinView::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    setActive(true);
}
void DolphinView::activate()
{
    setActive(true);
}

void DolphinView::triggerItem(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());

    const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    if ((modifier & Qt::ShiftModifier) || (modifier & Qt::ControlModifier)) {
        // items are selected by the user, hence don't trigger the
        // item specified by 'index'
        return;
    }

    const KFileItem item = m_dolphinModel->itemForIndex(m_proxyModel->mapToSource(index));

    if (item.isNull()) {
        return;
    }

    emit itemTriggered(item); // caught by DolphinViewContainer or DolphinPart
}

void DolphinView::generatePreviews(const QList<KFileItem>& items)
{
    if (m_controller->showPreview()) {
        KIO::PreviewJob* job = KIO::filePreview(items, 128);
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

    const QModelIndex idx = m_dolphinModel->indexForItem(item);
    if (idx.isValid() && (idx.column() == 0)) {
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            KIconEffect iconEffect;
            const QPixmap cutPixmap = iconEffect.apply(pixmap, K3Icon::Desktop, K3Icon::DisabledState);
            m_dolphinModel->setData(idx, QIcon(cutPixmap), Qt::DecorationRole);
        } else {
            m_dolphinModel->setData(idx, QIcon(pixmap), Qt::DecorationRole);
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
            emit errorMessage(i18nc("@info:status", "The location is empty."));
        } else {
            emit errorMessage(i18nc("@info:status", "The location '%1' is invalid.", location));
        }
        return;
    }

    m_cutItemsCache.clear();
    m_loadingDirectory = true;

    m_dirLister->stop();

    bool keepOldDirs = isColumnViewActive() && !m_initializeColumnView;
    m_initializeColumnView = false;

    if (keepOldDirs) {
        // keeping old directories is only necessary for hierarchical views
        // like the column view
        if (reload) {
            // for the column view it is not enough to reload the directory lister,
            // so this task is delegated to the column view directly
            m_columnView->reload();
        } else if (m_dirLister->directories().contains(url)) {
            // The dir lister contains the directory already, so
            // KDirLister::openUrl() may not get invoked twice.
            m_dirLister->updateDirectory(url);
        } else {
            const KUrl& dirListerUrl = m_dirLister->url();
            if ((dirListerUrl == url) || !m_dirLister->url().isParentOf(url)) {
                // The current URL is not a child of the dir lister
                // URL. This may happen when e. g. a place has been selected
                // and hence the view must be reset.
                m_dirLister->openUrl(url, false, false);
            }
        }
    } else {
        m_dirLister->openUrl(url, false, reload);
    }
}

KUrl DolphinView::viewPropertiesUrl() const
{
    if (isColumnViewActive()) {
        return m_dirLister->url();
    }

    return url();
}

void DolphinView::applyViewProperties(const KUrl& url)
{
    if (isColumnViewActive() && m_dirLister->url().isParentOf(url)) {
        // The column view is active, hence don't apply the view properties
        // of sub directories (represented by columns) to the view. The
        // view always represents the properties of the first column.
        return;
    }

    const ViewProperties props(url);

    const Mode mode = props.viewMode();
    if (m_mode != mode) {
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
        m_proxyModel->setCategorizedModel(categorized);
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

void DolphinView::changeSelection(const QList<KFileItem>& selection)
{
    clearSelection();
    if (selection.isEmpty()) {
        return;
    }
    const KUrl& baseUrl = url();
    KUrl url;
    QItemSelection new_selection;
    foreach(const KFileItem& item, selection) {
        url = item.url().upUrl();
        if (baseUrl.equals(url, KUrl::CompareWithoutTrailingSlash)) {
            QModelIndex index = m_proxyModel->mapFromSource(m_dolphinModel->indexForItem(item));
            new_selection.select(index, index);
        }
    }
    itemView()->selectionModel()->select(new_selection,
                                         QItemSelectionModel::ClearAndSelect
                                         | QItemSelectionModel::Current);
}

void DolphinView::openContextMenu(const QPoint& pos)
{
    KFileItem item;

    const QModelIndex index = itemView()->indexAt(pos);
    if (isValidNameIndex(index)) {
        item = fileItem(index);
    }

    emit requestContextMenu(item, url());
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const KUrl& destPath,
                           const QModelIndex& destIndex,
                           QWidget* source)
{
    KFileItem directory;
    if (isValidNameIndex(destIndex)) {
        KFileItem item = fileItem(destIndex);
        Q_ASSERT(!item.isNull());
        if (item.isDir()) {
            // the URLs are dropped above a directory
            directory = item;
        }
    }

    if ((directory.isNull()) && (source == itemView())) {
        // The dropping is done into the same viewport where
        // the dragging has been started. Just ignore this...
        return;
    }

    const KUrl& destination = (directory.isNull()) ?
                              destPath : directory.url();
    dropUrls(urls, destination);
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const KUrl& destination)
{
    emit urlsDropped(urls, destination);
}

void DolphinView::updateSorting(DolphinView::Sorting sorting)
{
    ViewProperties props(viewPropertiesUrl());
    props.setSorting(sorting);

    m_proxyModel->setSorting(sorting);

    emit sortingChanged(sorting);
}

void DolphinView::updateSortOrder(Qt::SortOrder order)
{
    ViewProperties props(viewPropertiesUrl());
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
        const QModelIndex index = m_dolphinModel->indexForUrl((*it).url);
        if (index.isValid()) {
            m_dolphinModel->setData(index, QIcon((*it).pixmap), Qt::DecorationRole);
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

    const KFileItem item = fileItem(index);
    if (!item.isNull()) {
        emit requestItemInfo(item);
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
        m_iconsView->setCategoryDrawer(new DolphinCategoryDrawer());
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

    new KMimeTypeResolver(view, m_dolphinModel);
    m_topLayout->insertWidget(1, view);

    connect(view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(emitSelectionChangedSignal()));
    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
    view->setFocus();
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
    return index.isValid() && (index.column() == DolphinModel::Name);
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
            const QModelIndex index = m_dolphinModel->indexForItem(*item);
            // Huh? the item is already known
            //const KFileItem item = m_dolphinModel->itemForIndex(index);
            const QVariant value = m_dolphinModel->data(index, Qt::DecorationRole);
            if (value.type() == QVariant::Icon) {
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
                m_dolphinModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
        ++it;
    }
}

void DolphinView::updateViewportColor()
{
    QColor color = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    if (m_active) {
        emit urlChanged(url()); // Hmm, this is a hack; the url hasn't really changed.
        emit selectionChanged(selectedItems());
    } else {
        color.setAlpha(0);
    }

    QWidget* viewport = itemView()->viewport();
    QPalette palette;
    palette.setColor(viewport->backgroundRole(), color);
    viewport->setPalette(palette);
}

#include "dolphinview.moc"
