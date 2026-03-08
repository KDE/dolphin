/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincolumnsview.h"

#include "dolphin_columnsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphincolumnpane.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "tooltips/tooltipmanager.h"
#include <KIO/Global>
#include <QGraphicsSceneDragDropEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

DolphinColumnsView::DolphinColumnsView(const QUrl &url, QWidget *parent, std::optional<Mode> initialMode)
    : DolphinView(url, parent, initialMode)
{
    initColumnsUi();

    // Virtual dispatch does not work during the base-class constructor, so
    // DolphinColumnsView::applyModeToView() was never called if ViewProperties
    // already had ColumnsView.  Initialise the columns now that the subclass
    // is fully constructed.
    if (viewMode() == ColumnsView) {
        m_columnsInitialized = true;
        m_scrollArea->show();
        itemListContainer()->hide();

        // The base class sets setFocusProxy(m_container), which redirects
        // all setFocus() calls to the hidden base container.  In columns
        // mode we manage focus ourselves (active column pane).
        setFocusProxy(nullptr);

        // The base constructor called loadDirectory() on its own model.
        // In columns mode we use per-column models, so cancel the base
        // load to avoid signal interference (e.g. applyDynamicView).
        auto *baseModel = static_cast<KFileItemModel *>(itemListContainer()->controller()->model());
        baseModel->cancelDirectoryLoading();
        baseModel->clear();

        syncColumnsFromViewProperties();
        rebuildColumnsForUrl(url);
    }
}

DolphinColumnsView::~DolphinColumnsView()
{
    // Restore base model so the base destructor doesn't use a pane model
    // that will be deleted when columns are destroyed.
    setBaseModel(m_baseModel);
}

void DolphinColumnsView::initColumnsUi()
{
    m_baseModel = DolphinView::activeModel(); // save original base model

    m_columnsSelectionTimer = new QTimer(this);
    m_columnsSelectionTimer->setSingleShot(true);
    connect(m_columnsSelectionTimer, &QTimer::timeout, this, [this] {
        Q_EMIT selectionChanged(selectedItems());
    });

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);

    // Filler widget absorbs extra space so columns keep their sizes
    m_filler = new QWidget();
    m_filler->setMinimumWidth(0);
    m_splitter->addWidget(m_filler);
    m_splitter->setStretchFactor(0, 1);

    m_scrollArea->setWidget(m_splitter);
    topLayout()->addWidget(m_scrollArea);

    connect(m_splitter, &QSplitter::splitterMoved, this, &DolphinColumnsView::slotSplitterMoved);

    // Start hidden; applyModeToView() controls visibility based on mode
    m_scrollArea->hide();

    // Propagate property changes from DolphinView to all column panes
    // via the standard DolphinView signals (no duplicate implementation)
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
}

void DolphinColumnsView::setUrl(const QUrl &url)
{
    if (url == this->url()) {
        return;
    }

    updateUrl(url);
    rebuildColumnsForUrl(url);
    Q_EMIT urlChanged(url);
}

void DolphinColumnsView::setActive(bool active)
{
    DolphinView::setActive(active);

    // Override focus target: give focus to the active column pane
    if (active) {
        if (auto *pane = activePane()) {
            pane->container()->setFocus();
        }
    }
}

void DolphinColumnsView::reload()
{
    rebuildColumnsForUrl(url());
}

void DolphinColumnsView::stopLoading()
{
    for (auto *pane : std::as_const(m_columns)) {
        pane->model()->cancelDirectoryLoading();
    }
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

int DolphinColumnsView::activeColumnIndex() const
{
    return m_activeColumn;
}

void DolphinColumnsView::setActiveColumn(int index)
{
    if (index < 0 || index >= m_columns.size()) {
        return;
    }

    DolphinColumnPane *oldPane = activePane();

    m_activeColumn = index;

    DolphinColumnPane *newPane = m_columns.at(m_activeColumn);
    reconnectActivePane(oldPane, newPane);

    // Keep focus proxy in sync so external setFocus() calls (e.g. from
    // DolphinViewContainer::requestFocus) land on the correct pane.
    setFocusProxy(newPane->container());
    newPane->container()->setFocus();
    ensureActiveColumnVisible();

    updateUrl(newPane->dirUrl());
    Q_EMIT urlChanged(newPane->dirUrl());
}

void DolphinColumnsView::applyModeToView()
{
    // Show columns UI, hide standard container
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
    // Apply current view properties to all column panes
    for (auto *pane : std::as_const(m_columns)) {
        pane->setPreviewsShown(previewsShown());
        pane->model()->setShowHiddenFiles(hiddenFilesShown());
        pane->model()->setSortRole(sortRole());
        pane->model()->setSortOrder(sortOrder());
    }
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

    // Only react to the column the user is interacting with
    if (!senderPane->container()->hasFocus()) {
        return;
    }

    // During rubber band (multi-select) or mouse-driven selection the
    // current item changes as the mouse moves, but we must not navigate —
    // only select.  Mouse-click navigation is handled by mouseButtonPressed.
    if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
        return;
    }

    const int colIndex = m_columns.indexOf(senderPane);
    if (colIndex < 0) {
        return;
    }

    // Directory → open child column (no auto-select in the child)
    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (!folderUrl.isEmpty()) {
        openChild(colIndex, folderUrl);
    } else if (item.isDir()) {
        openChild(colIndex, item.url());
    } else {
        // File selected → pop child columns
        popAfter(colIndex);
        recalculateColumnWidths();
        m_activeColumn = colIndex;
        setFocusProxy(senderPane->container());
        updateUrl(m_columns.at(colIndex)->dirUrl());

        m_blockNavigation = true;
        senderPane->setActiveChildUrl(item.url());
        m_blockNavigation = false;

        Q_EMIT urlChanged(m_columns.at(colIndex)->dirUrl());
    }
}

void DolphinColumnsView::slotPaneLoadingCompleted()
{
    auto *pane = qobject_cast<DolphinColumnPane *>(sender());

    // If this pane was flagged for auto-selection (Right arrow opened a
    // new child column whose model was still loading), select the first
    // item and open its child/preview now that items are available.
    if (pane && pane == m_pendingAutoSelect) {
        m_pendingAutoSelect = nullptr;
        const int colIndex = m_columns.indexOf(pane);
        if (colIndex >= 0) {
            autoSelectFirstItem(colIndex);
        }
    }

    Q_EMIT directoryLoadingCompleted();
}

void DolphinColumnsView::slotSplitterMoved(int pos, int handleIndex)
{
    Q_UNUSED(pos)
    // handleIndex is 1-based (handle 1 = between widget 0 and widget 1)
    const int colIndex = handleIndex - 1;
    if (colIndex >= 0 && colIndex < m_columns.size()) {
        const int width = m_splitter->sizes().at(colIndex);
        m_customColumnWidths[colIndex] = width;
    }
}

DolphinColumnPane *DolphinColumnsView::activePane() const
{
    if (m_activeColumn >= 0 && m_activeColumn < m_columns.size()) {
        return m_columns.at(m_activeColumn);
    }
    return nullptr;
}

void DolphinColumnsView::rebuildColumnsForUrl(const QUrl &url)
{
    popAfter(-1);

    DolphinColumnPane *pane = createPane(url);
    m_columns.append(pane);

    // Insert before the filler (filler is always last)
    m_splitter->insertWidget(m_splitter->count() - 1, pane);
    m_splitter->setStretchFactor(m_splitter->indexOf(pane), 0);

    recalculateColumnWidths();
    setActiveColumn(0);
}

void DolphinColumnsView::openChild(int columnIndex, const QUrl &childUrl)
{
    popAfter(columnIndex);

    // Mark the parent's selected item; guard against cascading navigation
    // since setActiveChildUrl triggers slotCurrentChanged in the pane.
    auto *parentPane = m_columns.at(columnIndex);
    m_blockNavigation = true;
    parentPane->setActiveChildUrl(childUrl);
    m_blockNavigation = false;

    DolphinColumnPane *pane = createPane(childUrl);
    m_columns.append(pane);

    // Insert before the filler
    m_splitter->insertWidget(m_splitter->count() - 1, pane);
    m_splitter->setStretchFactor(m_splitter->indexOf(pane), 0);

    recalculateColumnWidths();

    QTimer::singleShot(0, this, &DolphinColumnsView::ensureActiveColumnVisible);
}

void DolphinColumnsView::popAfter(int columnIndex)
{
    // If the active pane is about to be destroyed, disconnect it first
    // to prevent queued signals from dereferencing a dangling pointer.
    DolphinColumnPane *currentActive = activePane();
    if (currentActive && m_activeColumn > columnIndex) {
        reconnectActivePane(currentActive, nullptr);
    }

    while (m_columns.size() > columnIndex + 1) {
        DolphinColumnPane *pane = m_columns.takeLast();
        if (pane == m_pendingAutoSelect) {
            m_pendingAutoSelect = nullptr;
        }
        pane->setParent(nullptr);
        pane->deleteLater();
    }

    // Remove custom widths for columns that no longer exist
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

    // Keep focus proxy in sync after columns were removed
    if (auto *pane = activePane()) {
        setFocusProxy(pane->container());
    }
}

DolphinColumnPane *DolphinColumnsView::createPane(const QUrl &dirUrl)
{
    // Create model externally and configure BEFORE loading.
    // Changing properties (e.g. hidden files) during an active load
    // causes KDirLister::emitChanges() to insert items twice.
    auto *model = new KFileItemModel();
    model->setShowHiddenFiles(hiddenFilesShown());
    model->setSortRole(sortRole());
    model->setSortOrder(sortOrder());

    auto *pane = new DolphinColumnPane(model, nullptr);
    pane->setPreviewsShown(previewsShown());
    model->loadDirectory(dirUrl);

    connect(pane, &DolphinColumnPane::fileActivated, this, &DolphinColumnsView::slotFileActivated);
    connect(pane, &DolphinColumnPane::directoryActivated, this, [this, pane](const QUrl &childUrl) {
        const int colIndex = m_columns.indexOf(pane);
        if (colIndex < 0) {
            return;
        }
        openChild(colIndex, childUrl);
        if (colIndex < m_columns.size() - 1) {
            const int childCol = colIndex + 1;
            if (m_columns.at(childCol)->model()->count() > 0) {
                autoSelectFirstItem(childCol);
            } else {
                m_pendingAutoSelect = m_columns.at(childCol);
            }
            setActiveColumn(childCol);
            updateUrl(m_columns.at(m_activeColumn)->dirUrl());
            Q_EMIT urlChanged(url());
        }
    });
    connect(pane, &DolphinColumnPane::currentItemChanged, this, &DolphinColumnsView::slotColumnsCurrentItemChanged);
    connect(pane, &DolphinColumnPane::directoryLoadingCompleted, this, &DolphinColumnsView::slotPaneLoadingCompleted);

    // Forward VCS observer messages
    connect(pane, &DolphinColumnPane::infoMessage, this, &DolphinView::infoMessage);
    connect(pane, &DolphinColumnPane::errorMessage, this, [this](const QString &msg) {
        Q_EMIT errorMessage(msg, KIO::ERR_UNKNOWN);
    });
    connect(pane, &DolphinColumnPane::operationCompletedMessage, this, &DolphinView::operationCompletedMessage);

    // Drop handling: reuse base class helper
    connect(pane->controller(), &KItemListController::itemDropEvent, this, [this, pane](int index, QGraphicsSceneDragDropEvent *event) {
        handleItemDropEvent(pane->model(), pane->dirUrl(), index, event);
    });

    connect(pane->controller(), &KItemListController::mouseButtonPressed, this, [this, pane](int itemIndex, Qt::MouseButtons buttons) {
        handleMouseButtonPressed(pane, itemIndex, buttons);
    });

    pane->container()->installEventFilter(this);
    // Also filter the viewport for mouse events
    pane->container()->viewport()->installEventFilter(this);

    // Install event filter on the splitter handle for double-click cycling.
    // Must be called after the pane is inserted into the splitter (done by caller).
    QTimer::singleShot(0, this, [this, pane] {
        const int handleIdx = m_splitter->indexOf(pane);
        if (handleIdx > 0) {
            m_splitter->handle(handleIdx)->installEventFilter(this);
        }
    });

    return pane;
}

bool DolphinColumnsView::eventFilter(QObject *watched, QEvent *event)
{
    // Splitter handle double-click: cycle column width
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_splitter) {
        for (int i = 1; i < m_splitter->count(); ++i) {
            if (m_splitter->handle(i) == watched) {
                cycleColumnWidth(i - 1);
                return true;
            }
        }
    }

    // Find which column this event belongs to (match container or its viewport)
    int sourceColumn = -1;
    for (int i = 0; i < m_columns.size(); ++i) {
        if (m_columns.at(i)->container() == watched || m_columns.at(i)->container()->viewport() == watched) {
            sourceColumn = i;
            break;
        }
    }

    if (sourceColumn < 0) {
        return DolphinView::eventFilter(watched, event);
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
    recalculateColumnWidths();
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
        updateUrl(m_columns.at(m_activeColumn)->dirUrl());
        Q_EMIT urlChanged(url());
    }
    // Always consumed to prevent leaking to the Places panel
}

void DolphinColumnsView::handleKeyRight(int sourceColumn)
{
    // If a child column already exists, move into it
    if (sourceColumn < m_columns.size() - 1) {
        auto *childPane = m_columns.at(sourceColumn + 1);
        if (childPane->model()->count() > 0) {
            // Only auto-select if the child has no existing selection
            if (!childPane->controller()->selectionManager()->hasSelection()) {
                autoSelectFirstItem(sourceColumn + 1);
            }
        } else {
            // Model still loading — auto-select once ready
            m_pendingAutoSelect = childPane;
        }
        setActiveColumn(sourceColumn + 1);
        updateUrl(m_columns.at(m_activeColumn)->dirUrl());
        Q_EMIT urlChanged(url());
        return;
    }

    // No child column yet, try to open the current item as one
    auto *pane = m_columns.at(sourceColumn);
    const int current = pane->controller()->selectionManager()->currentItem();
    if (current >= 0 && current < pane->model()->count()) {
        const KFileItem item = pane->model()->fileItem(current);
        if (!item.isNull()) {
            const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
            if (!folderUrl.isEmpty()) {
                openChild(sourceColumn, folderUrl);
            } else if (item.isDir()) {
                openChild(sourceColumn, item.url());
            }
        }
    }

    // Post-openChild: activate the new child column + auto-select
    if (sourceColumn < m_columns.size() - 1) {
        const int childCol = sourceColumn + 1;
        if (m_columns.at(childCol)->model()->count() > 0) {
            autoSelectFirstItem(childCol);
        } else {
            m_pendingAutoSelect = m_columns.at(childCol);
        }
        setActiveColumn(childCol);
        updateUrl(m_columns.at(m_activeColumn)->dirUrl());
        Q_EMIT urlChanged(url());
    }
}

bool DolphinColumnsView::handleKeyReturn(int sourceColumn)
{
    auto *pane = m_columns.at(sourceColumn);
    const int current = pane->controller()->selectionManager()->currentItem();
    if (current < 0 || current >= pane->model()->count()) {
        return false;
    }

    const KFileItem item = pane->model()->fileItem(current);
    if (item.isNull()) {
        return false;
    }

    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (!folderUrl.isEmpty()) {
        openChild(sourceColumn, folderUrl);
    } else if (item.isDir()) {
        openChild(sourceColumn, item.url());
    } else {
        Q_EMIT itemActivated(item);
        return true;
    }

    // Navigate into the newly opened child column
    if (sourceColumn < m_columns.size() - 1) {
        const int childCol = sourceColumn + 1;
        if (m_columns.at(childCol)->model()->count() > 0) {
            autoSelectFirstItem(childCol);
        } else {
            m_pendingAutoSelect = m_columns.at(childCol);
        }
        setActiveColumn(childCol);
        updateUrl(m_columns.at(m_activeColumn)->dirUrl());
        Q_EMIT urlChanged(url());
    }
    return true;
}

void DolphinColumnsView::handleMouseButtonPressed(DolphinColumnPane *pane, int itemIndex, Qt::MouseButtons buttons)
{
    hideToolTip();

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

    // Activate clicked column on any mouse button so that right-click
    // on an inactive column activates it before the context menu.
    if (colIndex != m_activeColumn) {
        setActiveColumn(colIndex);
        updateUrl(pane->dirUrl());
        Q_EMIT urlChanged(url());
    }

    if (!(buttons & Qt::LeftButton)) {
        return;
    }

    // If clicking an already-current item, currentChanged won't fire.
    // Handle directory navigation manually for this case.
    if (itemIndex != pane->controller()->selectionManager()->currentItem()) {
        return;
    }
    if (itemIndex >= pane->model()->count()) {
        return;
    }

    const KFileItem item = pane->model()->fileItem(itemIndex);
    if (item.isNull()) {
        return;
    }

    QUrl targetUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (targetUrl.isEmpty() && item.isDir()) {
        targetUrl = item.url();
    }
    if (targetUrl.isEmpty()) {
        return;
    }

    // Only open child if not already showing this URL
    if (colIndex + 1 < m_columns.size() && m_columns.at(colIndex + 1)->dirUrl() == targetUrl) {
        return;
    }
    openChild(colIndex, targetUrl);
}

void DolphinColumnsView::ensureActiveColumnVisible()
{
    if (m_columns.isEmpty()) {
        return;
    }
    QWidget *activeWidget = activePane();
    if (!activeWidget) {
        return;
    }
    QWidget *rightTarget = m_columns.last();

    const int activeLeft = activeWidget->mapTo(m_splitter, QPoint(0, 0)).x();
    const int rightEdge = rightTarget->mapTo(m_splitter, QPoint(rightTarget->width(), 0)).x();
    const int viewportWidth = m_scrollArea->viewport()->width();

    int scrollValue = m_scrollArea->horizontalScrollBar()->value();

    // Try to show the right edge of the rightmost content
    if (rightEdge > scrollValue + viewportWidth) {
        scrollValue = rightEdge - viewportWidth;
    }

    // Ensure the active column's left edge is visible (takes priority)
    if (activeLeft < scrollValue) {
        scrollValue = activeLeft;
    }

    m_scrollArea->horizontalScrollBar()->setValue(scrollValue);
}

void DolphinColumnsView::autoSelectFirstItem(int columnIndex)
{
    if (columnIndex < 0 || columnIndex >= m_columns.size()) {
        return;
    }

    auto *pane = m_columns.at(columnIndex);
    auto *selectionManager = pane->controller()->selectionManager();

    // Ensure the current item is set and selected.  Guard against cascading
    // navigation from the programmatic selection change.
    if (pane->model()->count() > 0) {
        m_blockNavigation = true;
        if (selectionManager->currentItem() < 0) {
            selectionManager->setCurrentItem(0);
        }
        // Always re-apply selection; it may have been cleared by Left arrow
        selectionManager->setSelected(selectionManager->currentItem(), 1, KItemListSelectionManager::Select);
        m_blockNavigation = false;
    }

    // Trigger navigation for the selected item directly, since
    // m_blockNavigation suppressed signal-driven navigation above.
    const int current = selectionManager->currentItem();
    if (current < 0 || current >= pane->model()->count()) {
        return;
    }

    const KFileItem item = pane->model()->fileItem(current);
    if (item.isNull()) {
        return;
    }

    Q_EMIT requestItemInfo(item);

    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (!folderUrl.isEmpty()) {
        openChild(columnIndex, folderUrl);
    } else if (item.isDir()) {
        openChild(columnIndex, item.url());
    }
}

void DolphinColumnsView::recalculateColumnWidths()
{
    const int numColumns = m_columns.size();
    if (numColumns == 0) {
        return;
    }

    const int viewportWidth = m_scrollArea ? m_scrollArea->viewport()->width() : width();
    if (viewportWidth <= 0) {
        // Constructor time, viewport not yet laid out
        return;
    }

    const int divisor = qMin(ColumnsModeSettings::self()->maxVisibleColumns(), numColumns);
    const int defaultWidth = qMax(ColumnsModeSettings::self()->minColumnWidth(), viewportWidth / divisor);

    QList<int> sizes;
    sizes.reserve(m_splitter->count());
    for (int i = 0; i < numColumns; ++i) {
        if (m_customColumnWidths.contains(i)) {
            sizes.append(m_customColumnWidths.value(i));
        } else {
            sizes.append(defaultWidth);
        }
    }
    sizes.append(0); // filler

    m_splitter->setSizes(sizes);

    int totalWidth = 0;
    for (int i = 0; i < numColumns; ++i) {
        totalWidth += sizes.at(i);
    }
    totalWidth += (numColumns - 1) * m_splitter->handleWidth();

    if (totalWidth > viewportWidth) {
        m_splitter->setMinimumWidth(totalWidth);
    } else {
        m_splitter->setMinimumWidth(0);
    }
}

void DolphinColumnsView::cycleColumnWidth(int colIndex)
{
    if (colIndex < 0 || colIndex >= m_columns.size()) {
        return;
    }

    const int currentWidth = m_splitter->sizes().at(colIndex);
    const int optimalWidth = m_columns.at(colIndex)->calculateOptimalWidth();
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int divisor = qMin(ColumnsModeSettings::self()->maxVisibleColumns(), m_columns.size());
    const int defaultWidth = qMax(ColumnsModeSettings::self()->minColumnWidth(), viewportWidth / divisor);
    const bool hasCustom = m_customColumnWidths.contains(colIndex);
    const int customWidth = hasCustom ? m_customColumnWidths.value(colIndex) : 0;

    int newWidth;
    const int tolerance = 5;

    // Cycle: Optimal -> Default -> Custom (if available) -> Optimal ...
    if (qAbs(currentWidth - optimalWidth) <= tolerance) {
        newWidth = defaultWidth;
    } else if (qAbs(currentWidth - defaultWidth) <= tolerance) {
        newWidth = (hasCustom && qAbs(customWidth - defaultWidth) > tolerance && qAbs(customWidth - optimalWidth) > tolerance) ? customWidth : optimalWidth;
    } else if (hasCustom && qAbs(currentWidth - customWidth) <= tolerance) {
        newWidth = optimalWidth;
    } else {
        newWidth = optimalWidth;
    }

    QList<int> sizes = m_splitter->sizes();
    sizes[colIndex] = newWidth;
    m_splitter->setSizes(sizes);

    int totalWidth = 0;
    for (int i = 0; i < m_columns.size(); ++i) {
        totalWidth += sizes.at(i);
    }
    totalWidth += (m_columns.size() - 1) * m_splitter->handleWidth();
    m_splitter->setMinimumWidth(totalWidth > m_scrollArea->viewport()->width() ? totalWidth : 0);
}

void DolphinColumnsView::reconnectActivePane(DolphinColumnPane *oldPane, DolphinColumnPane *newPane)
{
    // Disconnect old pane's controller/selectionManager signals from this
    if (oldPane) {
        auto *controller = oldPane->controller();
        auto *selectionManager = controller->selectionManager();
        disconnect(selectionManager, &KItemListSelectionManager::selectionChanged, this, nullptr);
        disconnect(controller, &KItemListController::itemContextMenuRequested, this, nullptr);
        disconnect(controller, &KItemListController::viewContextMenuRequested, this, nullptr);
        disconnect(controller, &KItemListController::itemMiddleClicked, this, nullptr);
        disconnect(controller, &KItemListController::itemHovered, this, nullptr);
        disconnect(controller, &KItemListController::itemUnhovered, this, nullptr);
        disconnect(controller, &KItemListController::escapePressed, this, nullptr);
        disconnect(controller, &KItemListController::increaseZoom, this, nullptr);
        disconnect(controller, &KItemListController::decreaseZoom, this, nullptr);
        disconnect(controller, &KItemListController::swipeUp, this, nullptr);
        disconnect(controller, &KItemListController::doubleClickViewBackground, this, nullptr);
        disconnect(controller, &KItemListController::selectionModeChangeRequested, this, nullptr);
    }

    if (!newPane) {
        setBaseModel(m_baseModel);
        return;
    }

    // Point base-class m_model at the active pane's model so that
    // selectedItems(), fileItem(), etc. work correctly.
    setBaseModel(newPane->model());

    auto *controller = newPane->controller();
    auto *selectionManager = controller->selectionManager();

    // Selection changed with timer batching (like base view)
    connect(selectionManager, &KItemListSelectionManager::selectionChanged, this, [this, newPane](const KItemSet &current, const KItemSet &previous) {
        const bool stateChanged = (current.isEmpty() != previous.isEmpty());
        m_columnsSelectionTimer->setInterval(stateChanged ? 0 : 300);
        m_columnsSelectionTimer->start();

        if (current.count() != 1) {
            return;
        }

        auto currentColumnIndex = m_columns.indexOf(newPane);
        auto item = newPane->model()->fileItem(current.first());
        if (item.isNull() || !item.isDir()) {
            popAfter(currentColumnIndex);
            return;
        }

        if (currentColumnIndex != -1) {
            newPane->controller()->selectionManager()->blockSignals(true);
            openChild(currentColumnIndex, item.url());

            setActiveColumn(currentColumnIndex + 1);
            updateUrl(m_columns.at(m_activeColumn)->dirUrl());
            Q_EMIT urlChanged(url());

            newPane->controller()->selectionManager()->blockSignals(false);
        }
    });

    connect(controller, &KItemListController::itemContextMenuRequested, this, [this, newPane](int index, const QPointF &pos) {
        if (m_columnsSelectionTimer->isActive()) {
            m_columnsSelectionTimer->stop();
            Q_EMIT selectionChanged(selectedItems());
        }
        const KFileItem item = newPane->model()->fileItem(index);
        Q_EMIT requestContextMenu(pos.toPoint(), item, selectedItems(), newPane->dirUrl());
    });

    connect(controller, &KItemListController::viewContextMenuRequested, this, [this, newPane](const QPointF &pos) {
        Q_EMIT requestContextMenu(pos.toPoint(), KFileItem(), selectedItems(), newPane->dirUrl());
    });

    connect(controller, &KItemListController::itemMiddleClicked, this, [this, newPane](int index) {
        const KFileItem item = newPane->model()->fileItem(index);
        const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, GeneralSettings::browseThroughArchives());
        const auto modifiers = QGuiApplication::keyboardModifiers();
        if (!folderUrl.isEmpty()) {
            if (modifiers & Qt::ShiftModifier) {
                Q_EMIT activeTabRequested(folderUrl);
            } else {
                Q_EMIT tabRequested(folderUrl);
            }
        } else if (isTabsForFilesEnabled()) {
            if (modifiers & Qt::ShiftModifier) {
                Q_EMIT activeTabRequested(item.url());
            } else {
                Q_EMIT tabRequested(item.url());
            }
        } else {
            Q_EMIT fileMiddleClickActivated(item);
        }
    });

    connect(controller, &KItemListController::itemHovered, this, [this, newPane, controller](int index) {
        const KFileItem item = newPane->model()->fileItem(index);
        if (GeneralSettings::showToolTips() && !isDragging()) {
            QRectF itemRect = controller->view()->itemContextRect(index);
            const QPoint pos = newPane->container()->mapToGlobal(itemRect.topLeft().toPoint());
            itemRect.moveTo(pos);
#if HAVE_BALOO
            auto nativeParent = nativeParentWidget();
            if (nativeParent && toolTipManager()) {
                toolTipManager()->showToolTip(item, itemRect, nativeParent->windowHandle());
            }
#endif
        }
        Q_EMIT requestItemInfo(item);
    });

    connect(controller, &KItemListController::itemUnhovered, this, [this](int) {
        hideToolTip();
        Q_EMIT requestItemInfo(KFileItem());
    });

    connect(controller, &KItemListController::escapePressed, this, &DolphinView::stopLoading);

    connect(controller, &KItemListController::increaseZoom, this, [this] {
        setZoomLevel(zoomLevel() + 1);
    });
    connect(controller, &KItemListController::decreaseZoom, this, [this] {
        setZoomLevel(zoomLevel() - 1);
    });

    connect(controller, &KItemListController::swipeUp, this, [this] {
        Q_EMIT goUpRequested();
    });

    connect(controller, &KItemListController::doubleClickViewBackground, this, &DolphinView::doubleClickViewBackground);

    connect(controller, &KItemListController::selectionModeChangeRequested, this, &DolphinView::selectionModeChangeRequested);
}
