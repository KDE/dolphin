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
#include <QClipboard>

#include <kcolorscheme.h>
#include <kdirlister.h>
#include <kfileitemdelegate.h>
#include <klocale.h>
#include <kiconeffect.h>
#include <kio/deletejob.h>
#include <kio/netaccess.h>
#include <kio/previewjob.h>
#include <kjob.h>
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
    m_showPreview(false),
    m_loadingDirectory(false),
    m_storedCategorizedSorting(false),
    m_mode(DolphinView::IconsView),
    m_topLayout(0),
    m_controller(0),
    m_iconsView(0),
    m_detailsView(0),
    m_columnView(0),
    m_fileItemDelegate(0),
    m_selectionModel(0),
    m_dolphinModel(dolphinModel),
    m_dirLister(dirLister),
    m_proxyModel(proxyModel),
    m_previewJob(0)
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
    connect(m_controller, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&, const KFileItem&)),
            this, SLOT(dropUrls(const KUrl::List&, const KUrl&, const KFileItem&)));
    connect(m_controller, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(updateSorting(DolphinView::Sorting)));
    connect(m_controller, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(updateSortOrder(Qt::SortOrder)));
    connect(m_controller, SIGNAL(additionalInfoChanged(const KFileItemDelegate::InformationList&)),
            this, SLOT(updateAdditionalInfo(const KFileItemDelegate::InformationList&)));
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
    if (m_previewJob != 0) {
        m_previewJob->kill();
        m_previewJob = 0;
    }
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
    m_selectionModel->clearSelection();

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

    deleteView();

    // It is important to read the view properties _after_ deleting the view,
    // as e. g. the detail view might adjust the additional information properties
    // after getting closed:
    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setViewMode(m_mode);
    createView();

    // the file item delegate has been recreated, apply the current
    // additional information manually
    const KFileItemDelegate::InformationList infoList = props.additionalInfo();
    m_fileItemDelegate->setShowInformation(infoList);
    emit additionalInfoChanged(infoList);

    // Not all view modes support categorized sorting. Adjust the sorting model
    // if changing the view mode results in a change of the categorized sorting
    // capabilities.
    m_storedCategorizedSorting = props.categorizedSorting();
    const bool categorized = m_storedCategorizedSorting && supportsCategorizedSorting();
    if (categorized != m_proxyModel->isCategorizedModel()) {
        m_proxyModel->setCategorizedModel(categorized);
        emit categorizedSortingChanged();
    }

    emit modeChanged();
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setShowPreview(bool show)
{
    if (m_showPreview == show) {
        return;
    }

    const KUrl viewPropsUrl = viewPropertiesUrl();
    ViewProperties props(viewPropsUrl);
    props.setShowPreview(show);

    m_showPreview = show;

    emit showPreviewChanged();

    loadDirectory(viewPropsUrl, true);
}

bool DolphinView::showPreview() const
{
    return m_showPreview;
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
    m_fileItemDelegate->setShowInformation(info);

    emit additionalInfoChanged(info);

    if (itemView() != m_detailsView) {
        // the details view requires no reloading of the directory, as it maps
        // the file item delegate info to its columns internally
        loadDirectory(viewPropsUrl, true);
    }
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

    emit startedPathLoading(url);
}

void DolphinView::setNameFilter(const QString& nameFilter)
{
    m_proxyModel->setFilterRegExp(nameFilter);

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
    if (m_controller->dolphinView()->showPreview()) {
        if (m_previewJob != 0) {
            m_previewJob->kill();
            m_previewJob = 0;
        }

        m_previewJob = KIO::filePreview(items, 128);
        connect(m_previewJob, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                this, SLOT(replaceIcon(const KFileItem&, const QPixmap&)));
        connect(m_previewJob, SIGNAL(finished(KJob*)),
                this, SLOT(slotPreviewJobFinished(KJob*)));
    }
}

void DolphinView::replaceIcon(const KFileItem& item, const QPixmap& pixmap)
{
    Q_ASSERT(!item.isNull());
    if (!m_showPreview || (item.url().directory() != m_dirLister->url().path())) {
        // the preview has been deactivated in the meanwhile or the preview
        // job is still working on items of an older URL, hence
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
        emit showHiddenFilesChanged();
    }

    m_storedCategorizedSorting = props.categorizedSorting();
    const bool categorized = m_storedCategorizedSorting && supportsCategorizedSorting();
    if (categorized != m_proxyModel->isCategorizedModel()) {
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

    KFileItemDelegate::InformationList info = props.additionalInfo();
    if (info != m_fileItemDelegate->showInformation()) {
        m_fileItemDelegate->setShowInformation(info);
        emit additionalInfoChanged(info);
    }

    const bool showPreview = props.showPreview();
    if (showPreview != m_showPreview) {
        m_showPreview = showPreview;
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
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        item = fileItem(index);
    }

    emit requestContextMenu(item, url());
}

void DolphinView::dropUrls(const KUrl::List& urls,
                           const KUrl& destPath,
                           const KFileItem& destItem)
{
    const KUrl& destination = !destItem.isNull() && destItem.isDir() ?
                              destItem.url() : destPath;
    const KUrl sourceDir = KUrl(urls.first().directory());
    if (sourceDir != destination) {
        dropUrls(urls, destination);
    }
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

void DolphinView::updateAdditionalInfo(const KFileItemDelegate::InformationList& info)
{
    ViewProperties props(viewPropertiesUrl());
    props.setAdditionalInfo(info);
    props.save();

    m_fileItemDelegate->setShowInformation(info);

    emit additionalInfoChanged(info);

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
    if (hasSelection() || !m_active) {
        return;
    }

    emit requestItemInfo(item);
}

void DolphinView::clearHoverInformation()
{
    if (m_active) {
        emit requestItemInfo(KFileItem());
    }
}


void DolphinView::createView()
{
    deleteView();
    Q_ASSERT(m_iconsView == 0);
    Q_ASSERT(m_detailsView == 0);
    Q_ASSERT(m_columnView == 0);

    QAbstractItemView* view = 0;
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
    view->setItemDelegate(m_fileItemDelegate);

    view->setModel(m_proxyModel);
    if (m_selectionModel != 0) {
        view->setSelectionModel(m_selectionModel);
    } else {
        m_selectionModel = view->selectionModel();
    }

    // reparent the selection model, as it should not be deleted
    // when deleting the model
    m_selectionModel->setParent(this);

    view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    new KMimeTypeResolver(view, m_dolphinModel);
    m_topLayout->insertWidget(1, view);

    connect(view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(emitSelectionChangedSignal()));
    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(emitContentsMoved()));
}

void DolphinView::deleteView()
{
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

void DolphinView::renameSelectedItems()
{
    const KFileItemList items = selectedItems();
    if (items.count() > 1) {
        // More than one item has been selected for renaming. Open
        // a rename dialog and rename all items afterwards.
        RenameDialog dialog(this, items);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString newName = dialog.newName();
        if (newName.isEmpty()) {
            emit errorMessage(dialog.errorString());
        } else {
            // TODO: check how this can be integrated into KonqFileUndoManager/KonqOperations
            // as one operation instead of n rename operations like it is done now...
            Q_ASSERT(newName.contains('#'));

            // iterate through all selected items and rename them...
            const int replaceIndex = newName.indexOf('#');
            Q_ASSERT(replaceIndex >= 0);
            int index = 1;

            KFileItemList::const_iterator it = items.begin();
            const KFileItemList::const_iterator end = items.end();
            while (it != end) {
                const KUrl& oldUrl = (*it).url();
                QString number;
                number.setNum(index++);

                QString name(newName);
                name.replace(replaceIndex, 1, number);

                if (oldUrl.fileName() != name) {
                    KUrl newUrl = oldUrl;
                    newUrl.setFileName(name);
                    KonqOperations::rename(this, oldUrl, newUrl);
                    emit doingOperation(KonqFileUndoManager::RENAME);
                }
                ++it;
            }
        }
    } else {
        // Only one item has been selected for renaming. Use the custom
        // renaming mechanism from the views.
        Q_ASSERT(items.count() == 1);

        // TODO: Think about using KFileItemDelegate as soon as it supports editing.
        // Currently the RenameDialog is used, but I'm not sure whether inline renaming
        // is a benefit for the user at all -> let's wait for some input first...
        RenameDialog dialog(this, items);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            emit errorMessage(dialog.errorString());
        } else {
            const KUrl& oldUrl = items.first().url();
            KUrl newUrl = oldUrl;
            newUrl.setFileName(newName);
            KonqOperations::rename(this, oldUrl, newUrl);
            emit doingOperation(KonqFileUndoManager::RENAME);
        }
    }
}

void DolphinView::trashSelectedItems()
{
    emit doingOperation(KonqFileUndoManager::TRASH);
    KonqOperations::del(this, KonqOperations::TRASH, selectedUrls());
}

void DolphinView::deleteSelectedItems()
{
    const KUrl::List list = selectedUrls();
    const bool del = KonqOperations::askDeleteConfirmation(list,
                     KonqOperations::DEL,
                     KonqOperations::DEFAULT_CONFIRMATION,
                     this);

    if (del) {
        KIO::Job* job = KIO::del(list);
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotDeleteFileFinished(KJob*)));
    }
}

void DolphinView::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        emit operationCompletedMessage(i18nc("@info:status", "Delete operation completed."));
    } else {
        emit errorMessage(job->errorString());
    }
}

void DolphinView::slotPreviewJobFinished(KJob* job)
{
    Q_ASSERT(job == m_previewJob);
    m_previewJob = 0;
}

void DolphinView::cutSelectedItems()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinView::copySelectedItems()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinView::paste()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    const KUrl::List sourceUrls = KUrl::List::fromMimeData(mimeData);

    // per default the pasting is done into the current Url of the view
    KUrl destUrl(url());

    // check whether the pasting should be done into a selected directory
    const KUrl::List selectedUrls = this->selectedUrls();
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
        KonqOperations::copy(this, KonqOperations::MOVE, sourceUrls, destUrl);
        emit doingOperation(KonqFileUndoManager::MOVE);
        clipboard->clear();
    } else {
        KonqOperations::copy(this, KonqOperations::COPY, sourceUrls, destUrl);
        emit doingOperation(KonqFileUndoManager::COPY);
    }
}

QPair<bool, QString> DolphinView::pasteInfo() const
{
    QPair<bool, QString> ret;
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    if (!urls.isEmpty()) {
        ret.first = true;
        ret.second = i18ncp("@action:inmenu", "Paste One File", "Paste %1 Files", urls.count());
    } else {
        ret.first = false;
        ret.second = i18nc("@action:inmenu", "Paste");
    }

    if (ret.first) {
        const KUrl::List urls = selectedUrls();
        const uint count = urls.count();
        if (count > 1) {
            // pasting should not be allowed when more than one file
            // is selected
            ret.first = false;
        } else if (count == 1) {
            // Only one file is selected. Pasting is only allowed if this
            // file is a directory.
            // TODO: this doesn't work with remote protocols; instead we need a
            // m_activeViewContainer->selectedFileItems() to get the real KFileItems
            const KFileItem fileItem(S_IFDIR,
                                     KFileItem::Unknown,
                                     urls.first(),
                                     true);
            ret.first = fileItem.isDir();
        }
    }
    return ret;
}

KAction* DolphinView::createRenameAction(KActionCollection* collection)
{
    KAction* rename = collection->addAction("rename");
    rename->setText(i18nc("@action:inmenu File", "Rename..."));
    rename->setShortcut(Qt::Key_F2);
    return rename;
}

KAction* DolphinView::createMoveToTrashAction(KActionCollection* collection)
{
    KAction* moveToTrash = collection->addAction("move_to_trash");
    moveToTrash->setText(i18nc("@action:inmenu File", "Move to Trash"));
    moveToTrash->setIcon(KIcon("user-trash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    return moveToTrash;
}

KAction* DolphinView::createDeleteAction(KActionCollection* collection)
{
    KAction* deleteAction = collection->addAction("delete");
    deleteAction->setIcon(KIcon("edit-delete"));
    deleteAction->setText(i18nc("@action:inmenu File", "Delete"));
    deleteAction->setShortcut(Qt::SHIFT | Qt::Key_Delete);
    return deleteAction;
}

#include "dolphinview.moc"
