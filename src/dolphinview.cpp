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
#include <ktoggleaction.h>
#include <kactioncollection.h>

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

DolphinView::DolphinView(QWidget* parent,
                         const KUrl& url,
                         KDirLister* dirLister,
                         DolphinModel* dolphinModel,
                         DolphinSortFilterProxyModel* proxyModel) :
    QWidget(parent),
    m_active(true),
    m_loadingDirectory(false),
    m_storedCategorizedSorting(false),
    m_mode(DolphinView::IconsView),
    m_topLayout(0),
    m_controller(0),
    m_iconsView(0),
    m_detailsView(0),
    m_columnView(0),
    m_fileItemDelegate(0),
    m_dolphinModel(dolphinModel),
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

    // Receiver of the DolphinView signal 'urlChanged()' don't need
    // to care whether the internal controller changed the URL already or whether
    // the controller just requested an URL change and will be updated later.
    // In both cases the URL has been changed:
    connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
            this, SIGNAL(urlChanged(const KUrl&)));
    connect(m_controller, SIGNAL(requestUrlChange(const KUrl&)),
            this, SIGNAL(urlChanged(const KUrl&)));

    connect(m_controller, SIGNAL(requestContextMenu(const QPoint&)),
            this, SLOT(openContextMenu(const QPoint&)));
    connect(m_controller, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&, const QModelIndex&, QWidget*)),
            this, SLOT(dropUrls(const KUrl::List&, const KUrl&, const QModelIndex&, QWidget*)));
    connect(m_controller, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(updateSorting(DolphinView::Sorting)));
    connect(m_controller, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(updateSortOrder(Qt::SortOrder)));
    connect(m_controller, SIGNAL(itemTriggered(const KFileItem&)),
            this, SLOT(triggerItem(const KFileItem&)));
    connect(m_controller, SIGNAL(activated()),
            this, SLOT(activate()));
    connect(m_controller, SIGNAL(itemEntered(const KFileItem&)),
            this, SLOT(showHoverInformation(const KFileItem&)));
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
    return isColumnViewActive() ? m_columnView->rootUrl() : url();
}

void DolphinView::setActive(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    QColor color = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    if (active) {
        // TODO: emitting urlChanged() is a hack, as the URL hasn't really changed. It
        // bypasses the problem when having a split view and changing the active view to
        // update the some URL dependent states. A nicer approach should be no big deal...
        emit urlChanged(url());
        emit selectionChanged(selectedItems());
    } else {
        color.setAlpha(150);
    }

    QWidget* viewport = itemView()->viewport();
    QPalette palette;
    palette.setColor(viewport->backgroundRole(), color);
    viewport->setPalette(palette);

    update();

    if (active) {
        emit activated();
    }

    m_controller->indicateActivationChange(active);
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
        const KUrl root = rootUrl();
        setUrl(root);
        m_controller->setUrl(root);
    }

    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setViewMode(m_mode);

    createView();

    // Not all view modes support categorized sorting. Adjust the sorting model
    // if changing the view mode results in a change of the categorized sorting
    // capabilities.
    m_storedCategorizedSorting = props.categorizedSorting();
    const bool categorized = m_storedCategorizedSorting && supportsCategorizedSorting();
    if (categorized != m_proxyModel->isCategorizedModel()) {
        m_proxyModel->setCategorizedModel(categorized);
        m_proxyModel->sort(m_proxyModel->sortColumn(), m_proxyModel->sortOrder());
        emit categorizedSortingChanged();
    }

    loadDirectory(viewPropsUrl);

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

    loadDirectory(viewPropsUrl, true);
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
    m_controller->setShowHiddenFiles(show);
    emit showHiddenFilesChanged();

    loadDirectory(viewPropsUrl, true);
}

bool DolphinView::showHiddenFiles() const
{
    return m_dirLister->showingDotFiles();
}

void DolphinView::setCategorizedSorting(bool categorized)
{
    if (categorized == categorizedSorting()) {
        return;
    }

    // setCategorizedSorting(true) may only get invoked
    // if the view supports categorized sorting
    Q_ASSERT(!categorized || supportsCategorizedSorting());

    ViewProperties props(viewPropertiesUrl());
    props.setCategorizedSorting(categorized);
    props.save();

    m_storedCategorizedSorting = categorized;
    m_proxyModel->setCategorizedModel(categorized);
    m_proxyModel->sort(m_proxyModel->sortColumn(), m_proxyModel->sortOrder());

    emit categorizedSortingChanged();
}

bool DolphinView::categorizedSorting() const
{
    // If all view modes would support categorized sorting, returning
    // m_proxyModel->isCategorizedModel() would be the way to go. As
    // currently only the icons view supports caterized sorting, we remember
    // the stored view properties state in m_storedCategorizedSorting and
    // return this state. The application takes care to disable the corresponding
    // checkbox by checking DolphinView::supportsCategorizedSorting() to indicate
    // that this setting is not applied to the current view mode.
    return m_storedCategorizedSorting;
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
    if (isColumnViewActive()) {
        // QAbstractItemView does not offer a virtual method invertSelection()
        // as counterpart to QAbstractItemView::selectAll(). This makes it
        // necessary to delegate the inverting of the selection to the
        // column view, as only the selection of the active column should
        // get inverted.
        m_columnView->invertSelection();
    } else {
        QItemSelectionModel* selectionModel = itemView()->selectionModel();
        const QAbstractItemModel* itemModel = selectionModel->model();

        const QModelIndex topLeft = itemModel->index(0, 0);
        const QModelIndex bottomRight = itemModel->index(itemModel->rowCount() - 1,
                                                         itemModel->columnCount() - 1);

        const QItemSelection selection(topLeft, bottomRight);
        selectionModel->select(selection, QItemSelectionModel::Toggle);
    }
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

    // Our view has a selection, we will map them back to the DolphinModel
    // and then fill the KFileItemList.
    Q_ASSERT((view != 0) && (view->selectionModel() != 0));

    const QItemSelection selection = m_proxyModel->mapSelectionToSource(view->selectionModel()->selection());
    KFileItemList itemList;

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
    const KFileItemList list = selectedItems();
    foreach (KFileItem item, list) {
        urls.append(item.url());
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

void DolphinView::setAdditionalInfo(KFileItemDelegate::InformationList info)
{
    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setAdditionalInfo(info);

    m_controller->setAdditionalInfoCount(info.count());
    m_fileItemDelegate->setShowInformation(info);

    emit additionalInfoChanged(info);
    loadDirectory(viewPropsUrl, true);
}

KFileItemDelegate::InformationList DolphinView::additionalInfo() const
{
    return m_fileItemDelegate->showInformation();
}

void DolphinView::reload()
{
    setUrl(url());
    loadDirectory(url(), true);
}

void DolphinView::refresh()
{
    const bool oldActivationState = m_active;
    m_active = true;

    createView();
    applyViewProperties(m_controller->url());
    reload();

    setActive(oldActivationState);
}

void DolphinView::updateView(const KUrl& url, const KUrl& rootUrl)
{
    if (m_controller->url() == url) {
        return;
    }

    m_controller->setUrl(url); // emits urlChanged, which we forward

    if (!rootUrl.isEmpty() && rootUrl.isParentOf(url)) {
        applyViewProperties(rootUrl);
        loadDirectory(rootUrl);
        if (itemView() == m_columnView) {
            m_columnView->setRootUrl(rootUrl);
            m_columnView->showColumn(url);
        }
    } else {
        applyViewProperties(url);
        loadDirectory(url);
    }

    itemView()->setFocus();

    emit startedPathLoading(url);
}

void DolphinView::setNameFilter(const QString& nameFilter)
{
    // The name filter of KDirLister does a 'hard' filtering, which
    // means that only the items are shown where the names match
    // exactly the filter. This is non-transparent for the user, which
    // just wants to have a 'soft' filtering: does the name contain
    // the filter string?
    QString adjustedFilter(nameFilter);
    adjustedFilter.insert(0, '*');
    adjustedFilter.append('*');

    m_dirLister->setNameFilter(adjustedFilter);
    m_dirLister->emitChanges();

    if (isColumnViewActive()) {
        // adjusting the directory lister is not enough in the case of the
        // column view, as each column has its own directory lister internally...
        m_columnView->setNameFilter(nameFilter);
    }
}

void DolphinView::calculateItemCount(int& fileCount, int& folderCount)
{
    foreach (KFileItem item, m_dirLister->items()) {
        if (item.isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
        }
    }
}

void DolphinView::setUrl(const KUrl& url)
{
    updateView(url, KUrl());
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

void DolphinView::triggerItem(const KFileItem& item)
{
    const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    if ((modifier & Qt::ShiftModifier) || (modifier & Qt::ControlModifier)) {
        // items are selected by the user, hence don't trigger the
        // item specified by 'index'
        return;
    }

    if (item.isNull()) {
        return;
    }

    emit itemTriggered(item); // caught by DolphinViewContainer or DolphinPart
}

void DolphinView::generatePreviews(const KFileItemList& items)
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
            const QPixmap cutPixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
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

void DolphinView::loadDirectory(const KUrl& url, bool reload)
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
    m_dirLister->openUrl(url, reload ? KDirLister::Reload : KDirLister::NoFlags);

    if (isColumnViewActive()) {
        // adjusting the directory lister is not enough in the case of the
        // column view, as each column has its own directory lister internally...
        if (reload) {
            m_columnView->reload();
        } else {
            m_columnView->showColumn(url);
        }
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
    if (isColumnViewActive() && rootUrl().isParentOf(url)) {
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
    }
    if (itemView() == 0) {
        createView();
    }
    Q_ASSERT(itemView() != 0);
    Q_ASSERT(m_fileItemDelegate != 0);

    const bool showHiddenFiles = props.showHiddenFiles();
    if (showHiddenFiles != m_dirLister->showingDotFiles()) {
        m_dirLister->setShowingDotFiles(showHiddenFiles);
        m_controller->setShowHiddenFiles(showHiddenFiles);
        emit showHiddenFilesChanged();
    }

    m_storedCategorizedSorting = props.categorizedSorting();
    const bool categorized = m_storedCategorizedSorting && supportsCategorizedSorting();
    if (categorized != m_proxyModel->isCategorizedModel()) {
        m_proxyModel->setCategorizedModel(categorized);
        m_proxyModel->sort(m_proxyModel->sortColumn(), m_proxyModel->sortOrder());
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

    KFileItemDelegate::InformationList info = props.additionalInfo();
    if (info != m_fileItemDelegate->showInformation()) {
        m_controller->setAdditionalInfoCount(info.count());
        m_fileItemDelegate->setShowInformation(info);
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

void DolphinView::showHoverInformation(const KFileItem& item)
{
    if (hasSelection()) {
        return;
    }

    emit requestItemInfo(item);
}

void DolphinView::clearHoverInformation()
{
    emit requestItemInfo(KFileItem());
}


void DolphinView::createView()
{
    KFileItemDelegate::InformationList infoList;
    if (m_fileItemDelegate != 0) {
        infoList = m_fileItemDelegate->showInformation();
    }

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
    case IconsView: {
        m_iconsView = new DolphinIconsView(this, m_controller);
        view = m_iconsView;
        break;
    }

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
    m_fileItemDelegate->setShowInformation(infoList);
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
        const KFileItem item = *it;
        if (isCutItem(item)) {
            const QModelIndex index = m_dolphinModel->indexForItem(item);
            const QVariant value = m_dolphinModel->data(index, Qt::DecorationRole);
            if (value.type() == QVariant::Icon) {
                const QIcon icon(qvariant_cast<QIcon>(value));
                QPixmap pixmap = icon.pixmap(128, 128);

                // remember current pixmap for the item to be able
                // to restore it when other items get cut
                CutItem cutItem;
                cutItem.url = item.url();
                cutItem.pixmap = pixmap;
                m_cutItemsCache.append(cutItem);

                // apply icon effect to the cut item
                KIconEffect iconEffect;
                pixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
                m_dolphinModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
        ++it;
    }
}

KToggleAction* DolphinView::iconsModeAction(KActionCollection* actionCollection)
{
    KToggleAction* iconsView = actionCollection->add<KToggleAction>("icons");
    iconsView->setText(i18nc("@action:inmenu View Mode", "Icons"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("fileview-icon"));
    iconsView->setData(QVariant::fromValue(IconsView));
    return iconsView;
}

KToggleAction* DolphinView::detailsModeAction(KActionCollection* actionCollection)
{
    KToggleAction* detailsView = actionCollection->add<KToggleAction>("details");
    detailsView->setText(i18nc("@action:inmenu View Mode", "Details"));
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("fileview-detailed"));
    detailsView->setData(QVariant::fromValue(DetailsView));
    return detailsView;
}

KToggleAction* DolphinView::columnsModeAction(KActionCollection* actionCollection)
{
    KToggleAction* columnView = actionCollection->add<KToggleAction>("columns");
    columnView->setText(i18nc("@action:inmenu View Mode", "Columns"));
    columnView->setShortcut(Qt::CTRL | Qt::Key_3);
    columnView->setIcon(KIcon("fileview-column"));
    columnView->setData(QVariant::fromValue(ColumnView));
    return columnView;
}

QString DolphinView::currentViewModeActionName() const
{
    switch (m_mode) {
    case DolphinView::IconsView:
        return "icons";
    case DolphinView::DetailsView:
        return "details";
    case DolphinView::ColumnView:
        return "columns";
    }
    return QString(); // can't happen
}

#include "dolphinview.moc"
