/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincolumnsview.h"

#include "dolphin_columnsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphincolumnpane.h"
#include "dolphinitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "tooltips/tooltipmanager.h"
#include <KIO/Global>
#include <QApplication>
#include <QGraphicsSceneDragDropEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QTimer>
#include <QVBoxLayout>

/// How long a new column waits for its folder to list before it takes a width anyway.
static constexpr int s_pendingWidthTimeoutMs = 200;

DolphinColumnsView::DolphinColumnsView(const QUrl &url, QWidget *parent, std::optional<Mode> initialMode, std::function<QUrl(const QUrl &)> rootUrlResolver)
    : DolphinView(url, parent, initialMode, true)
    , m_rootUrlResolver(std::move(rootUrlResolver))
{
    initColumnsUi();

    // The base constructor cannot reach the applyModeToView() override.
    if (viewMode() == ColumnsView) {
        m_columnsInitialized = true;
        m_scrollArea->show();
        itemListContainer()->hide();

        // The focus goes to the active column, not to the hidden base container.
        setFocusProxy(nullptr);

        // Every column has its own model, so the listing of the base one is not wanted.
        auto *baseModel = static_cast<KFileItemModel *>(itemListContainer()->controller()->model());
        baseModel->cancelDirectoryLoading();
        baseModel->clear();

        syncColumnsFromViewProperties();
        rebuildColumnsForUrl(url);
    }
}

DolphinColumnsView::~DolphinColumnsView()
{
    // The models of the columns are deleted with them.
    setBaseModel(m_baseModel);
}

void DolphinColumnsView::initColumnsUi()
{
    m_baseModel = DolphinView::activeModel();

    // Every column is filtered the same way, starting from the filter of the base model.
    m_nameFilter = m_baseModel->nameFilter();
    m_filterMode = m_baseModel->filterMode();
    m_filterCaseSensitive = m_baseModel->isFilterCaseSensitive();

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);

    // Takes the room that the columns leave, so they keep their sizes.
    m_filler = new QWidget();
    m_filler->setMinimumWidth(0);
    m_splitter->addWidget(m_filler);
    m_splitter->setStretchFactor(0, 1);

    m_scrollArea->setWidget(m_splitter);
    topLayout()->addWidget(m_scrollArea);

    connect(m_splitter, &QSplitter::splitterMoved, this, &DolphinColumnsView::slotSplitterMoved);

    m_scrollArea->hide();

    connect(this, &DolphinView::previewsShownChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::hiddenFilesShownChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::sortRoleChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::sortOrderChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::groupedSortingChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::groupRoleChanged, this, [this] {
        syncColumnsFromViewProperties();
    });
    connect(this, &DolphinView::zoomLevelChanged, this, [this]() {
        for (auto c : std::as_const(m_columns)) {
            c->setZoomLevel(zoomLevel());
        }
        // Larger icons need wider columns.
        recalculateColumnWidths();
    });
}

void DolphinColumnsView::setUrl(const QUrl &url)
{
    if (url == this->url()) {
        return;
    }

    const QUrl previousUrl = this->url();
    updateUrl(url);
    if (!showUrlInOpenColumns(url, previousUrl)) {
        rebuildColumnsForUrl(url);
    }
    Q_EMIT urlChanged(url);
}

void DolphinColumnsView::setActive(bool active)
{
    DolphinView::setActive(active);

    if (active) {
        if (auto *pane = activePane()) {
            pane->container()->setFocus();
        }
    }
}

int DolphinColumnsView::horizontalScrollBarHeight() const
{
    if (m_scrollArea && m_scrollArea->horizontalScrollBar() && m_scrollArea->horizontalScrollBar()->isVisible()) {
        return m_scrollArea->horizontalScrollBar()->height();
    }
    return DolphinView::horizontalScrollBarHeight();
}

void DolphinColumnsView::setStatusBarOffset(int offset)
{
    m_statusBarOffset = offset;
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        if (KItemListView *view = pane->controller()->view()) {
            view->setStatusBarOffset(offset);
        }
    }
}

bool DolphinColumnsView::urlChangeLeavesTheSelectionBehind() const
{
    return !m_switchingColumns;
}

KItemListController *DolphinColumnsView::draggingController() const
{
    for (const DolphinColumnPane *pane : m_columns) {
        if (pane->controller()->isDragging()) {
            return pane->controller();
        }
    }
    return nullptr;
}

bool DolphinColumnsView::handleSpaceAsNormalKey() const
{
    // The base view asks its own container, which never has the focus here.
    if (auto *pane = activePane()) {
        return !pane->container()->hasFocus() || pane->controller()->isSearchAsYouTypeActive();
    }
    return DolphinView::handleSpaceAsNormalKey();
}

void DolphinColumnsView::reload()
{
    // Rebuilding would keep only the active column.
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        pane->model()->refreshDirectory(pane->dirUrl());
    }
}

void DolphinColumnsView::stopLoading()
{
    for (auto *pane : std::as_const(m_columns)) {
        pane->model()->cancelDirectoryLoading();
    }
}

void DolphinColumnsView::setNameFilter(const QString &nameFilter)
{
    m_nameFilter = nameFilter;
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        pane->model()->setNameFilter(nameFilter);
    }
    DolphinView::setNameFilter(nameFilter);
}

void DolphinColumnsView::setFilterMode(KFileItemModelFilter::FilterMode mode)
{
    m_filterMode = mode;
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        pane->model()->setFilterMode(mode);
    }
    DolphinView::setFilterMode(mode);
}

void DolphinColumnsView::setFilterCaseSensitive(bool caseSensitive)
{
    m_filterCaseSensitive = caseSensitive;
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        pane->model()->setFilterCaseSensitive(caseSensitive);
    }
    DolphinView::setFilterCaseSensitive(caseSensitive);
}

void DolphinColumnsView::readSettings()
{
    DolphinView::readSettings();

    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        pane->reloadSettings();
    }

    recalculateColumnWidths(WidthPolicy::Refit);
}

KFileItem DolphinColumnsView::rootItem() const
{
    if (auto *pane = activePane()) {
        return pane->model()->rootItem();
    }
    return DolphinView::rootItem();
}

void DolphinColumnsView::paste()
{
    if (auto *pane = activePane()) {
        pasteToUrl(pane->dirUrl());
    }
}

KItemListSelectionManager *DolphinColumnsView::activeSelectionManager() const
{
    if (auto *pane = activePane()) {
        return pane->controller()->selectionManager();
    }
    return DolphinView::activeSelectionManager();
}

KFileItemModel *DolphinColumnsView::activeModel() const
{
    if (auto *pane = activePane()) {
        return pane->model();
    }
    return DolphinView::activeModel();
}

DolphinItemListView *DolphinColumnsView::activeItemListView() const
{
    if (auto *pane = activePane()) {
        return pane->itemListView();
    }
    return DolphinView::activeItemListView();
}

int DolphinColumnsView::columnCount() const
{
    return m_columns.size();
}

DolphinColumnPane *DolphinColumnsView::columnAt(int index) const
{
    if (index >= 0 && index < m_columns.size()) {
        return m_columns.at(index);
    }
    return nullptr;
}

bool DolphinColumnsView::hasFocusInside() const
{
    const QWidget *focusWidget = QApplication::focusWidget();
    return focusWidget && (focusWidget == this || isAncestorOf(focusWidget));
}

int DolphinColumnsView::activeColumnIndex() const
{
    return m_activeColumn;
}

void DolphinColumnsView::setActiveColumn(int index)
{
    if (index < 0 || index >= m_columns.size()) {
        return;
    }

    const DolphinColumnPane *previousPane = activePane();
    const bool hadSelection = previousPane && previousPane->controller()->selectionManager()->hasSelection();
    m_activeColumn = index;

    DolphinColumnPane *newPane = m_columns.at(m_activeColumn);
    connectActivePane(newPane);

    // For setFocus() calls from outside, such as DolphinViewContainer::requestFocus().
    setFocusProxy(newPane->container());
    if (hasFocusInside()) {
        newPane->container()->setFocus();
    }
    ensureActiveColumnVisible();

    updateUrl(newPane->dirUrl());
    {
        QScopedValueRollback<bool> switching(m_switchingColumns, true);
        Q_EMIT urlChanged(newPane->dirUrl());
    }
    // A column selects its first item before it is entered, and only the active column reports its selection.
    if (hadSelection || newPane->controller()->selectionManager()->hasSelection()) {
        scheduleSelectionChangedSignal();
    }

    updateWritableState();
}

void DolphinColumnsView::applyModeToView()
{
    if (m_scrollArea) {
        m_scrollArea->show();
    }
    itemListContainer()->hide();
    setFocusProxy(nullptr); // columns manage focus themselves

    if (!m_columnsInitialized) {
        m_columnsInitialized = true;
        syncColumnsFromViewProperties();
        rebuildColumnsForUrl(url());
    }
}

void DolphinColumnsView::syncColumnsFromViewProperties()
{
    for (auto *pane : std::as_const(m_columns)) {
        pane->setPreviewsShown(previewsShown());
        applyViewProperties(pane->model());
    }
}

void DolphinColumnsView::applyViewProperties(KFileItemModel *model) const
{
    model->setShowHiddenFiles(hiddenFilesShown());
    model->setSortRole(sortRole());
    model->setSortOrder(sortOrder());
    model->setGroupedSorting(groupedSorting());
    model->setGroupRole(rawGroupRole());
}

void DolphinColumnsView::slotFileActivated(const KFileItem &item)
{
    Q_EMIT itemActivated(item);
}

void DolphinColumnsView::slotColumnsCurrentItemChanged(const KFileItem &item)
{
    if (m_blockNavigation) {
        return;
    }

    auto *senderPane = qobject_cast<DolphinColumnPane *>(sender());
    if (!senderPane) {
        return;
    }

    if (!senderPane->container()->hasFocus()) {
        return;
    }

    // A click navigates on the release, see handleMouseButtonReleased().
    if (QGuiApplication::mouseButtons() != Qt::NoButton) {
        return;
    }

    const int colIndex = m_columns.indexOf(senderPane);
    if (colIndex < 0) {
        return;
    }

    senderPane->controller()->selectionManager()->blockSignals(true);
    if (!followItem(colIndex, item)) {
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        senderPane->setActiveChildUrl(item.url());
    }
    senderPane->controller()->selectionManager()->blockSignals(false);
}

void DolphinColumnsView::slotPaneItemsInserted()
{
    for (DolphinColumnPane *pane : std::as_const(m_columns)) {
        if (pane->model() == sender()) {
            // Not a selection by the user, so it must not navigate.
            QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
            pane->reapplyActiveChildMark();
            return;
        }
    }
}

void DolphinColumnsView::slotPaneLoadingCompleted()
{
    auto *pane = qobject_cast<DolphinColumnPane *>(sender());

    if (pane) {
        // Marking the child is not a selection by the user, so it must not navigate.
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        pane->reapplyActiveChildMark();
    }

    if (pane) {
        finishPendingWidth(pane);
    }

    if (pane && pane == m_pendingAutoSelect) {
        m_pendingAutoSelect = nullptr;
        const int colIndex = m_columns.indexOf(pane);
        if (colIndex >= 0) {
            autoSelectFirstItem(colIndex);
        }
    }

    // The base view follows the writable state of the base model, which lists nothing here.
    updateWritableState();

    // As DolphinView::slotDirectoryLoadingCompleted() does, for the items marked to select, such as a created folder.
    if (pane && pane == activePane()) {
        QTimer::singleShot(0, this, &DolphinColumnsView::updateViewState);
    }

    Q_EMIT directoryLoadingCompleted();
}

void DolphinColumnsView::slotSplitterMoved(int pos, int handleIndex)
{
    Q_UNUSED(pos)
    // A drag resizes the columns on both sides of the handle, so both keep their new width.
    const QList<int> sizes = m_splitter->sizes();
    for (const int colIndex : {handleIndex - 1, handleIndex}) {
        if (colIndex >= 0 && colIndex < m_columns.size() && colIndex < sizes.size()) {
            m_customColumnWidths[colIndex] = sizes.at(colIndex);
        }
    }
}

DolphinColumnPane *DolphinColumnsView::activePane() const
{
    if (m_activeColumn >= 0 && m_activeColumn < m_columns.size()) {
        return m_columns.at(m_activeColumn);
    }
    return nullptr;
}

bool DolphinColumnsView::showUrlInOpenColumns(const QUrl &url, const QUrl &previousUrl)
{
    // KIO::upUrl() ends a folder url with a slash and dirUrl() does not.
    const QUrl target = url.adjusted(QUrl::StripTrailingSlash);

    for (int i = 0; i < m_columns.size(); ++i) {
        if (m_columns.at(i)->dirUrl().adjusted(QUrl::StripTrailingSlash) == target) {
            // Going up keeps the folder on the way back selected, as KCoreUrlNavigator::urlSelectionRequested()
            // asks the other view modes to. What was selected inside that folder is dropped.
            const bool nextIsOnTheWayBack = i + 1 < m_columns.size()
                && (m_columns.at(i + 1)->dirUrl().matches(previousUrl, QUrl::StripTrailingSlash) || m_columns.at(i + 1)->dirUrl().isParentOf(previousUrl));
            // Active first, so closing the columns after the next one leaves the url alone.
            setActiveColumn(i);
            m_columns.at(nextIsOnTheWayBack ? i + 1 : i)->dropActiveChild();
            return true;
        }
    }

    // Below one of the open columns, so open the folders between the two.
    int ancestor = -1;
    for (int i = m_columns.size() - 1; i >= 0; --i) {
        if (m_columns.at(i)->dirUrl().isParentOf(url)) {
            ancestor = i;
            break;
        }
    }
    if (ancestor < 0) {
        return false;
    }

    const QUrl ancestorUrl = m_columns.at(ancestor)->dirUrl().adjusted(QUrl::StripTrailingSlash);
    QList<QUrl> chain;
    for (QUrl step = target; step != ancestorUrl;) {
        chain.prepend(step);
        const QUrl parent = KIO::upUrl(step).adjusted(QUrl::StripTrailingSlash);
        if (parent == step) {
            // Not on one path after all.
            return false;
        }
        step = parent;
    }

    int column = ancestor;
    for (const QUrl &step : std::as_const(chain)) {
        openChild(column, step);
        ++column;
    }
    setActiveColumn(column);
    return true;
}

QUrl DolphinColumnsView::rootUrlFor(const QUrl &url) const
{
    if (!m_rootUrlResolver) {
        return url;
    }
    const QUrl root = m_rootUrlResolver(url);
    if (!root.isValid() || !(root.matches(url, QUrl::StripTrailingSlash) || root.isParentOf(url))) {
        return url;
    }
    return root;
}

void DolphinColumnsView::rebuildColumnsForUrl(const QUrl &url)
{
    // Removing the columns can move the focus out of the view.
    const bool hadFocus = hasFocusInside();
    popAfter(-1);

    const QUrl root = rootUrlFor(url);

    appendPane(root);
    recalculateColumnWidths();
    if (hadFocus) {
        m_columns.constFirst()->container()->setFocus();
    }
    setActiveColumn(0);

    if (root.adjusted(QUrl::StripTrailingSlash) != url.adjusted(QUrl::StripTrailingSlash)) {
        showUrlInOpenColumns(url);
    }
}

void DolphinColumnsView::openChild(int columnIndex, const QUrl &childUrl)
{
    // One interaction can reach this from both the current item and the selection.
    if (columnIndex + 1 < m_columns.size() && m_columns.at(columnIndex + 1)->dirUrl() == childUrl) {
        return;
    }

    // A column that replaces another starts at its width, so the view does not move.
    const bool replacesExistingColumn = columnIndex + 1 < m_columns.size();

    int carriedWidth = -1;
    if (columnIndex + 1 < m_columns.size()) {
        const QList<int> sizes = m_splitter->sizes();
        if (columnIndex + 1 < sizes.size()) {
            carriedWidth = sizes.at(columnIndex + 1);
        }
    }

    popAfter(columnIndex);

    auto *parentPane = m_columns.at(columnIndex);
    {
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        parentPane->setActiveChildUrl(childUrl);
    }

    DolphinColumnPane *pane = appendPane(childUrl);

    if (carriedWidth > 0) {
        m_carriedColumnWidth = {columnIndex + 1, carriedWidth};
    }

    // Sized before its folder lists, a column would jump to the width of its content.
    if (ColumnsModeSettings::self()->dynamicColumnWidth() && carriedWidth <= 0) {
        pane->setWidthPending(true);
        QTimer::singleShot(s_pendingWidthTimeoutMs, pane, [this, pane]() {
            finishPendingWidth(pane);
        });
    }

    recalculateColumnWidths();

    // A new column is scrolled to, once it has a width. The parent column stays active.
    if (pane->isWidthPending()) {
        // finishPendingWidth() scrolls to it.
    } else if (!replacesExistingColumn) {
        scrollToColumnWhenLaidOut(columnIndex + 1);
    } else {
        QTimer::singleShot(0, this, &DolphinColumnsView::ensureActiveColumnVisible);
    }
}

void DolphinColumnsView::finishPendingWidth(DolphinColumnPane *pane)
{
    if (pane->isWidthPending()) {
        pane->setWidthPending(false);
        recalculateColumnWidths();
        scrollToColumnWhenLaidOut(m_columns.indexOf(pane));
    }
}

void DolphinColumnsView::scrollToColumnWhenLaidOut(int index)
{
    // The splitter takes its new sizes on the next layout.
    const int activeWhenScheduled = m_activeColumn;
    QTimer::singleShot(0, this, [this, index, activeWhenScheduled]() {
        // The user may have moved to another column in the meantime.
        if (m_activeColumn != activeWhenScheduled && m_activeColumn != index) {
            ensureColumnVisible(m_activeColumn);
            return;
        }
        ensureColumnVisible(index);
    });
}

void DolphinColumnsView::popAfter(int columnIndex)
{
    // Disconnected first, so that no queued signal reaches a deleted column.
    if (activePane() && m_activeColumn > columnIndex) {
        connectActivePane(nullptr);
    }

    while (m_columns.size() > columnIndex + 1) {
        DolphinColumnPane *pane = m_columns.takeLast();
        if (pane == m_pendingAutoSelect) {
            m_pendingAutoSelect = nullptr;
        }
        pane->setParent(nullptr);
        disconnect(pane, nullptr, this, nullptr);
        disconnect(pane->controller(), nullptr, this, nullptr);
        KItemListController::deleteLaterAfterDrag(pane, pane->controller());
    }

    auto it = m_customColumnWidths.begin();
    while (it != m_customColumnWidths.end()) {
        if (it.key() > columnIndex) {
            it = m_customColumnWidths.erase(it);
        } else {
            ++it;
        }
    }

    if (m_activeColumn >= m_columns.size()) {
        m_activeColumn = m_columns.size() - 1;
    }

    if (auto *pane = activePane()) {
        setFocusProxy(pane->container());
    }
}

DolphinColumnPane *DolphinColumnsView::appendPane(const QUrl &dirUrl)
{
    // Configured before it loads: changing a property during a load inserts items twice.
    auto *model = new KFileItemModel();
    applyViewProperties(model);
    model->setFilterMode(m_filterMode);
    model->setFilterCaseSensitive(m_filterCaseSensitive);
    model->setNameFilter(m_nameFilter);

    auto *pane = new DolphinColumnPane(model, nullptr);
    if (KItemListView *paneView = pane->controller()->view()) {
        paneView->setStatusBarOffset(m_statusBarOffset);
    }
    pane->setPreviewsShown(previewsShown());
    pane->setZoomLevel(zoomLevel());
    model->loadDirectory(dirUrl);

    connect(pane, &DolphinColumnPane::fileActivated, this, &DolphinColumnsView::slotFileActivated);
    connect(pane, &DolphinColumnPane::directoryActivated, this, [this, pane](const QUrl &childUrl) {
        if (const int colIndex = m_columns.indexOf(pane); colIndex >= 0) {
            openChild(colIndex, childUrl);
        }
    });
    connect(pane, &DolphinColumnPane::currentItemChanged, this, &DolphinColumnsView::slotColumnsCurrentItemChanged);
    connect(pane, &DolphinColumnPane::activeChildRemoved, this, [this, pane]() {
        closeColumnsAfter(m_columns.indexOf(pane));
    });
    connect(pane, &DolphinColumnPane::directoryLoadingCompleted, this, &DolphinColumnsView::slotPaneLoadingCompleted);
    connect(model, &KFileItemModel::itemsInserted, this, &DolphinColumnsView::slotPaneItemsInserted);

    // A rename arrives as a "text" change from KIO, or as a remove and an insert from disk.
    connect(model, &KFileItemModel::itemsInserted, this, &DolphinColumnsView::refitColumnsToContent);
    connect(model, &KFileItemModel::itemsRemoved, this, &DolphinColumnsView::refitColumnsToContent);
    connect(model, &KFileItemModel::itemsChanged, this, [this](const KItemRangeList &, const QSet<QByteArray> &changedRoles) {
        if (changedRoles.contains("text")) {
            refitColumnsToContent();
        }
    });

    connect(pane, &DolphinColumnPane::infoMessage, this, &DolphinView::infoMessage);
    connect(pane, &DolphinColumnPane::errorMessage, this, [this](const QString &msg) {
        Q_EMIT errorMessage(msg, KIO::ERR_UNKNOWN);
    });
    connect(pane, &DolphinColumnPane::operationCompletedMessage, this, &DolphinView::operationCompletedMessage);

    pane->container()->installEventFilter(this);
    pane->container()->viewport()->installEventFilter(this);

    m_columns.append(pane);
    m_splitter->insertWidget(m_splitter->count() - 1, pane);
    m_splitter->setStretchFactor(m_splitter->indexOf(pane), 0);
    if (const int handleIndex = m_splitter->indexOf(pane); handleIndex > 0) {
        m_splitter->handle(handleIndex)->installEventFilter(this);
    }

    auto controller = pane->controller();
    connect(controller, &KItemListController::itemDropEvent, this, [this, pane](int index, QGraphicsSceneDragDropEvent *event) {
        // Dropped into the folder of the column, which becomes active so that the dropped items are selected in it.
        const KFileItem item = pane->model()->fileItem(index);
        if (item.isNull() || (!item.isDir() && !item.isDesktopFile() && !item.isExecutable())) {
            setActiveColumn(m_columns.indexOf(pane));
        }
        handleItemDropEvent(pane->model(), pane->dirUrl(), index, event);
    });

    connect(controller, &KItemListController::mouseButtonReleased, this, [this, pane](int itemIndex, Qt::MouseButtons buttons) {
        Q_UNUSED(buttons) // what is still held, which is nothing after a click
        handleMouseButtonReleased(pane, itemIndex);
    });
    connect(controller, &KItemListController::draggingStarted, this, [this]() {
        m_pressedPane = nullptr;
        m_pressedItemIndex = -1;
    });
    connect(controller, &KItemListController::mouseButtonPressed, this, [this, pane](int itemIndex, Qt::MouseButtons buttons) {
        handleMouseButtonPressed(pane, itemIndex, buttons);
    });

    connect(controller, &KItemListController::itemHovered, this, [this, pane](int index) {
        showHoveredItem(pane->container(), pane->model()->fileItem(index), index);
    });
    connect(controller, &KItemListController::itemUnhovered, this, &DolphinColumnsView::slotItemUnhovered);

    return pane;
}

bool DolphinColumnsView::eventFilter(QObject *watched, QEvent *event)
{
    // A double click on a handle fits the columns. Press and release still reach the handle.
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick) {
        for (int i = 1; i < m_splitter->count(); ++i) {
            if (m_splitter->handle(i) != watched) {
                continue;
            }
            switch (event->type()) {
            case QEvent::MouseButtonPress:
                m_splitterReleaseSeen = false;
                break;
            case QEvent::MouseButtonRelease:
                if (m_splitterReleaseSeen) {
                    // Not while the handle still holds the mouse grab.
                    QTimer::singleShot(0, this, &DolphinColumnsView::autoAdjustColumns);
                }
                m_splitterReleaseSeen = !m_splitterReleaseSeen;
                break;
            case QEvent::MouseButtonDblClick:
                m_splitterReleaseSeen = false;
                QTimer::singleShot(0, this, &DolphinColumnsView::autoAdjustColumns);
                return true;
            default:
                break;
            }
            break;
        }
    }

    int sourceColumn = -1;
    for (int i = 0; i < m_columns.size(); ++i) {
        if (m_columns.at(i)->container() == watched || m_columns.at(i)->container()->viewport() == watched) {
            sourceColumn = i;
            break;
        }
    }

    if (sourceColumn < 0) {
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        return DolphinView::eventFilter(watched, event);
    }

    // As the base view does for its container. This also covers a right-click on the background
    // of an inactive column, where handleMouseButtonPressed() has no item.
    if (event->type() == QEvent::FocusIn) {
        if (sourceColumn != m_activeColumn) {
            if (static_cast<QFocusEvent *>(event)->reason() == Qt::MouseFocusReason) {
                m_paneActiveBeforeFocus = activePane();
            }
            setActiveColumn(sourceColumn);
        }
        setActive(true);
    }

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Left) {
            handleKeyLeft(sourceColumn);
            return true;
        } else if (keyEvent->key() == Qt::Key_Right) {
            handleKeyRight(sourceColumn);
            return true;
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (handleKeyReturn(sourceColumn)) {
                return true;
            }
        }
    }

    return DolphinView::eventFilter(watched, event);
}

void DolphinColumnsView::resizeEvent(QResizeEvent *event)
{
    DolphinView::resizeEvent(event);
    if (event->size().width() > event->oldSize().width()) {
        showEarlierColumnsInTheRoomAfterTheLast();
    }
    recalculateColumnWidths(WidthPolicy::Refit);
}

void DolphinColumnsView::showEarlierColumnsInTheRoomAfterTheLast()
{
    if (!m_scrollArea || m_columns.isEmpty()) {
        return;
    }
    const QList<int> sizes = m_splitter->sizes();
    const int lastColumn = m_columns.size() - 1;
    int lastRight = visibleHandlesBefore(lastColumn) * m_splitter->handleWidth();
    for (int i = 0; i <= lastColumn && i < sizes.size(); ++i) {
        lastRight += sizes.at(i);
    }
    // ensureColumnVisible() then moves on to the left edge of a column.
    QScrollBar *scrollBar = m_scrollArea->horizontalScrollBar();
    const int scrollValue = qMax(0, lastRight - m_scrollArea->viewport()->width());
    if (scrollBar->value() > scrollValue) {
        scrollBar->setValue(scrollValue);
    }
}

void DolphinColumnsView::showEvent(QShowEvent *event)
{
    DolphinView::showEvent(event);
    recalculateColumnWidths();
}

void DolphinColumnsView::handleKeyLeft(int sourceColumn)
{
    if (sourceColumn > 0) {
        setActiveColumn(sourceColumn - 1);
    }
    // Consumed even in the first column, or the Places panel takes it.
}

void DolphinColumnsView::handleKeyRight(int sourceColumn)
{
    if (sourceColumn + 1 >= m_columns.size()) {
        const QUrl folderUrl = DolphinColumnPane::folderUrlFor(m_columns.at(sourceColumn)->currentFileItem());
        if (!folderUrl.isEmpty()) {
            openChild(sourceColumn, folderUrl);
        }
    }
    enterChildColumn(sourceColumn);
}

bool DolphinColumnsView::handleKeyReturn(int sourceColumn)
{
    // Several selected items are opened together by the controller, as in the other view modes.
    if (m_columns.at(sourceColumn)->controller()->selectionManager()->selectedItems().count() >= 2) {
        return false;
    }
    const KFileItem item = m_columns.at(sourceColumn)->currentFileItem();
    if (item.isNull()) {
        return false;
    }
    if (!followItem(sourceColumn, item)) {
        Q_EMIT itemActivated(item);
        return true;
    }
    enterChildColumn(sourceColumn);
    return true;
}

void DolphinColumnsView::enterChildColumn(int column)
{
    const int childColumn = column + 1;
    if (childColumn >= m_columns.size()) {
        return;
    }
    DolphinColumnPane *child = m_columns.at(childColumn);
    if (child->model()->count() == 0) {
        m_pendingAutoSelect = child;
    } else if (!child->controller()->selectionManager()->hasSelection()) {
        autoSelectFirstItem(childColumn);
    }
    setActiveColumn(childColumn);
}

bool DolphinColumnsView::followItem(int column, const KFileItem &item)
{
    const QUrl folderUrl = DolphinColumnPane::folderUrlFor(item);
    if (!folderUrl.isEmpty()) {
        openChild(column, folderUrl);
        return true;
    }
    // The columns to the right belong to a folder that is no longer the selected one.
    closeColumnsAfter(column);
    return false;
}

void DolphinColumnsView::closeColumnsAfter(int column)
{
    if (column < 0 || column + 1 >= m_columns.size()) {
        return;
    }
    const bool activeColumnIsAffected = m_activeColumn >= column;
    const bool activeColumnCloses = m_activeColumn > column;
    popAfter(column);
    recalculateColumnWidths();
    if (activeColumnIsAffected) {
        updateUrl(m_columns.at(column)->dirUrl());
        Q_EMIT urlChanged(url());
    }
    // The column left of the closed ones becomes active, and its selection was not reported.
    if (activeColumnCloses) {
        scheduleSelectionChangedSignal();
    }
}

void DolphinColumnsView::handleMouseButtonPressed(DolphinColumnPane *pane, int itemIndex, Qt::MouseButtons buttons)
{
    hideToolTip();
    m_pressedPane = nullptr;
    m_pressedItemIndex = -1;
    m_paneActiveBeforePress = m_paneActiveBeforeFocus ? m_paneActiveBeforeFocus.data() : activePane();
    m_paneActiveBeforeFocus = nullptr;

    if (buttons & Qt::BackButton) {
        Q_EMIT goBackRequested();
        return;
    } else if (buttons & Qt::ForwardButton) {
        Q_EMIT goForwardRequested();
        return;
    }

    if (itemIndex < 0) {
        return;
    }
    const int colIndex = m_columns.indexOf(pane);
    if (colIndex < 0) {
        return;
    }

    // Any button, so that a context menu belongs to the column.
    if (colIndex != m_activeColumn) {
        setActiveColumn(colIndex);
    }

    if (!(buttons & Qt::LeftButton)) {
        return;
    }

    // onPress() applies a Ctrl or Shift selection after this, which openChild() would clear.
    if (QGuiApplication::keyboardModifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
        return;
    }

    // Pressing an item that is part of a multi-selection starts a drag of the whole selection.
    auto *selectionManager = pane->controller()->selectionManager();
    if (selectionManager->selectedItems().count() > 1 && selectionManager->isSelected(itemIndex)) {
        return;
    }

    if (itemIndex >= pane->model()->count()) {
        return;
    }

    // Followed on the release, so that a drag finds the columns still open.
    m_pressedPane = pane;
    m_pressedItemIndex = itemIndex;
}

void DolphinColumnsView::handleMouseButtonReleased(DolphinColumnPane *pane, int itemIndex)
{
    if (!m_pressedPane || m_pressedPane != pane || m_pressedItemIndex != itemIndex) {
        return;
    }
    m_pressedPane = nullptr;
    m_pressedItemIndex = -1;

    const KFileItem item = pane->model()->fileItem(itemIndex);
    const int colIndex = m_columns.indexOf(pane);
    if (colIndex >= 0 && !item.isNull()) {
        followItem(colIndex, item);
    }

    // The item that was active stays highlighted only as an ancestor of the clicked one.
    const int previousColumn = m_columns.indexOf(m_paneActiveBeforePress);
    if (colIndex >= 0 && previousColumn > colIndex) {
        m_paneActiveBeforePress->controller()->selectionManager()->clearSelection();
    }
}

void DolphinColumnsView::ensureActiveColumnVisible()
{
    ensureColumnVisible(m_activeColumn);
}

void DolphinColumnsView::ensureColumnVisible(int index)
{
    if (index < 0 || index >= m_columns.size() || !m_scrollArea) {
        return;
    }
    QWidget *activeWidget = m_columns.at(index);

    // From the splitter sizes: a column just added has no geometry until the next layout.
    QList<int> sizes = m_splitter->sizes();
    const int handleWidth = m_splitter->handleWidth();
    int activeLeft = visibleHandlesBefore(index) * handleWidth;
    for (int i = 0; i < index && i < sizes.size(); ++i) {
        activeLeft += sizes.at(i);
    }
    const int activeWidth = index < sizes.size() ? sizes.at(index) : activeWidget->width();
    const int activeRight = activeLeft + activeWidth;
    const int viewportWidth = m_scrollArea->viewport()->width();

    QScrollBar *scrollBar = m_scrollArea->horizontalScrollBar();
    int scrollValue = scrollBar->value();

    if (activeRight > scrollValue + viewportWidth) {
        scrollValue = activeRight - viewportWidth;
    }

    // The first visible column starts at its left edge. Cut, it shows its selection without the name.
    // Also after the columns before it changed width, which leaves the scroll position inside one.
    int columnsWidth = 0;
    for (int column = 0; column <= index && column < sizes.size(); ++column) {
        const int left = columnsWidth + visibleHandlesBefore(column) * handleWidth;
        if (left >= scrollValue) {
            scrollValue = left;
            break;
        }
        columnsWidth += sizes.at(column);
    }

    // The left edge wins.
    if (activeLeft < scrollValue) {
        scrollValue = activeLeft;
    }

    // Starting at a column edge can need room past the last column, which the filler gives.
    if (scrollValue > scrollBar->maximum() && !sizes.isEmpty()) {
        sizes.last() += scrollValue - scrollBar->maximum();
        int totalWidth = visibleHandlesBefore(m_columns.size()) * handleWidth;
        for (int size : std::as_const(sizes)) {
            totalWidth += size;
        }
        m_splitter->setSizes(sizes);
        m_splitter->setMinimumWidth(totalWidth);
        m_splitter->resize(totalWidth, m_splitter->height());
    }

    scrollBar->setValue(scrollValue);
}

void DolphinColumnsView::autoSelectFirstItem(int columnIndex)
{
    if (columnIndex < 0 || columnIndex >= m_columns.size()) {
        return;
    }

    auto *pane = m_columns.at(columnIndex);
    auto *selectionManager = pane->controller()->selectionManager();

    if (pane->model()->count() > 0) {
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        if (selectionManager->currentItem() < 0) {
            selectionManager->setCurrentItem(0);
        }
        // Left may have cleared it.
        selectionManager->setSelected(selectionManager->currentItem(), 1, KItemListSelectionManager::Select);
    }

    // Like any selected folder, the first one shows its content in the next column.
    const KFileItem item = pane->currentFileItem();
    if (!item.isNull()) {
        followItem(columnIndex, item);
        Q_EMIT requestItemInfo(item);
    }
}

void DolphinColumnsView::refitColumnsToContent()
{
    if (ColumnsModeSettings::self()->dynamicColumnWidth()) {
        recalculateColumnWidths();
    }
}

void DolphinColumnsView::recalculateColumnWidths(WidthPolicy policy)
{
    const int numColumns = m_columns.size();
    if (numColumns == 0) {
        return;
    }

    const int viewportWidth = m_scrollArea ? m_scrollArea->viewport()->width() : width();
    if (viewportWidth <= 0) {
        return;
    }

    const int divisor = qMax(1, qMin(ColumnsModeSettings::self()->maxVisibleColumns(), numColumns));
    const int handleWidth = m_splitter->handleWidth();
    // Room for the handles, so a full set of columns fits without a scrollbar.
    const int availableForColumns = qMax(0, viewportWidth - (divisor - 1) * handleWidth);
    const int minColumnWidth = ColumnsModeSettings::self()->minColumnWidth();
    const int defaultWidth = qMax(minColumnWidth, availableForColumns / divisor);
    // One long name elides instead of pushing the other columns out of the viewport.
    const int maxVisibleColumns = ColumnsModeSettings::self()->maxVisibleColumns();
    const int maxContentWidth = qMax(minColumnWidth, qMax(0, viewportWidth - (maxVisibleColumns - 1) * handleWidth) / maxVisibleColumns);
    const bool dynamicWidth = ColumnsModeSettings::self()->dynamicColumnWidth();
    const QList<int> currentSizes = m_splitter->sizes();

    QList<int> sizes;
    sizes.reserve(m_splitter->count());
    for (int i = 0; i < numColumns; ++i) {
        int columnWidth;
        if (m_columns.at(i)->isWidthPending()) {
            sizes.append(0);
            continue;
        }
        if (m_customColumnWidths.contains(i)) {
            columnWidth = m_customColumnWidths.value(i);
        } else if (dynamicWidth) {
            columnWidth = qBound(minColumnWidth, m_columns.at(i)->calculateOptimalWidth(), maxContentWidth);
            if (policy == WidthPolicy::GrowOnly) {
                // Only the user narrows a column.
                columnWidth = qMax(columnWidth, widthFloorFor(i, currentSizes));
            }
        } else {
            columnWidth = defaultWidth;
        }
        // Never wider than the viewport, so scrolling to a column shows its scrollbar.
        sizes.append(qMin(viewportWidth, columnWidth));
    }
    applyColumnSizes(sizes);
}

int DolphinColumnsView::widthFloorFor(int index, const QList<int> &currentSizes) const
{
    int floor = 0;
    if (index < currentSizes.size()) {
        floor = currentSizes.at(index);
    }
    if (m_carriedColumnWidth && m_carriedColumnWidth->first == index) {
        floor = qMax(floor, m_carriedColumnWidth->second);
    }
    return floor;
}

int DolphinColumnsView::visibleHandlesBefore(int index) const
{
    int handles = 0;
    for (int i = 1; i <= index && i <= m_columns.size(); ++i) {
        // A column with no width yet has no handle either. Index m_columns.size() is the filler.
        if (i == m_columns.size() || !m_columns.at(i)->isWidthPending()) {
            ++handles;
        }
    }
    return handles;
}

void DolphinColumnsView::applyColumnSizes(QList<int> columnSizes)
{
    const int numColumns = m_columns.size();
    const int handleWidth = m_splitter->handleWidth();

    int totalWidth = 0;
    for (int i = 0; i < numColumns; ++i) {
        totalWidth += columnSizes.at(i);
    }
    // Including the handle before the filler, or the splitter takes a pixel from a column.
    totalWidth += visibleHandlesBefore(numColumns) * handleWidth;

    // The filler holds the scroll position when a column closes.
    const int viewportWidthForFiller = m_scrollArea ? m_scrollArea->viewport()->width() : width();
    const int scrollValue = m_scrollArea ? m_scrollArea->horizontalScrollBar()->value() : 0;
    const int filler = qMax(0, scrollValue + viewportWidthForFiller - totalWidth);
    totalWidth += filler;

    // Before setSizes(), so the layout counts the handles it draws. Handle 0 is not a separator.
    for (int i = 1; i < numColumns; ++i) {
        if (QSplitterHandle *handle = m_splitter->handle(i)) {
            handle->setVisible(!m_columns.at(i)->isWidthPending());
        }
    }

    columnSizes.append(filler);
    m_splitter->setSizes(columnSizes);

    // One handle of slack, for the viewport width that wobbles under fractional scaling.
    const int viewportWidth = m_scrollArea ? m_scrollArea->viewport()->width() : width();
    if (totalWidth > viewportWidth + handleWidth) {
        m_splitter->setMinimumWidth(totalWidth);
    } else {
        m_splitter->setMinimumWidth(0);
    }

    // The active column may have just taken its final width.
    ensureActiveColumnVisible();
}

void DolphinColumnsView::autoAdjustColumns()
{
    if (m_columns.isEmpty()) {
        return;
    }

    m_customColumnWidths.clear();
    const int minColumnWidth = ColumnsModeSettings::self()->minColumnWidth();
    const bool fixedWidth = !ColumnsModeSettings::self()->dynamicColumnWidth();

    QList<int> sizes;
    sizes.reserve(m_columns.size());
    for (int i = 0; i < m_columns.size(); ++i) {
        const int width = qMax(minColumnWidth, m_columns.at(i)->calculateOptimalWidth());
        sizes.append(width);
        if (fixedWidth) {
            // Or the next relayout would go back to the fixed width.
            m_customColumnWidths[i] = width;
        }
    }
    applyColumnSizes(sizes);
}

void DolphinColumnsView::connectActivePane(DolphinColumnPane *newPane)
{
    for (const QMetaObject::Connection &connection : std::as_const(m_activePaneConnections)) {
        disconnect(connection);
    }
    m_activePaneConnections.clear();

    if (!newPane) {
        setBaseModel(m_baseModel);
        return;
    }

    // For selectedItems(), fileItem() and the base slots.
    setBaseModel(newPane->model());

    m_activePaneConnections = connectItemController(newPane->controller());

    m_activePaneConnections.append(connect(newPane->controller()->selectionManager(),
                                           &KItemListSelectionManager::selectionChanged,
                                           this,
                                           &DolphinColumnsView::slotActiveSelectionChanged));
}

void DolphinColumnsView::slotActiveSelectionChanged(const KItemSet &current)
{
    // Not for a selection made by the view, such as openChild() marking the folder in its parent.
    if (m_blockNavigation || current.count() != 1) {
        return;
    }
    // A press selects the item, and a click navigates on the release, see handleMouseButtonReleased().
    if (QGuiApplication::mouseButtons() != Qt::NoButton) {
        return;
    }
    DolphinColumnPane *pane = activePane();
    const KFileItem item = pane->model()->fileItem(current.first());
    if (item.isDir()) {
        pane->controller()->selectionManager()->blockSignals(true);
        openChild(m_activeColumn, DolphinColumnPane::folderUrlFor(item));
        pane->controller()->selectionManager()->blockSignals(false);
    }
}
