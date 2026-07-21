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
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "tooltips/tooltipmanager.h"
#include "zoomlevelinfo.h"
#include <KIO/Global>
#include <QGraphicsSceneDragDropEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScopedValueRollback>
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
    connect(this, &DolphinView::zoomLevelChanged, this, [this]() {
        for (auto c : m_columns) {
            c->setZoomLevel(zoomLevel());
        }
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

void DolphinColumnsView::readSettings()
{
    DolphinView::readSettings();

    // With global view properties the configured icon/preview size is what every
    // column should use, so push it to the shared zoom level. Per-folder view
    // properties keep their own saved zoom, matching the other view modes.
    if (GeneralSettings::globalViewProps()) {
        const auto *settings = ColumnsModeSettings::self();
        const int iconSize = previewsShown() ? settings->previewSize() : settings->iconSize();
        setZoomLevel(ZoomLevelInfo::zoomLevelForIconSize(QSize(iconSize, iconSize)));
    }

    // Pick up changes to the width behaviour, minimum width, and visible-column
    // count that the settings dialog just applied.
    recalculateColumnWidths();
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

    // Track the writable state of the newly active pane's folder so the
    // write-dependent actions (Create New, paste, ...) reflect it.
    updateWritableState();
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

    // The current item also changes during mouse selection and on a
    // context-menu right-click; do not navigate then, only select. Mouse
    // navigation is handled by mouseButtonPressed.
    if (QGuiApplication::mouseButtons() != Qt::NoButton) {
        return;
    }

    const int colIndex = m_columns.indexOf(senderPane);
    if (colIndex < 0) {
        return;
    }

    senderPane->controller()->selectionManager()->blockSignals(true);

    // Directory → open child column (no auto-select in the child)
    const QUrl folderUrl = folderUrlForItem(item);
    if (!folderUrl.isEmpty()) {
        openChild(colIndex, folderUrl);
    } else {
        // File selected → pop child columns
        popAfter(colIndex);
        recalculateColumnWidths();
        m_activeColumn = colIndex;
        setFocusProxy(senderPane->container());
        updateUrl(m_columns.at(colIndex)->dirUrl());

        {
            QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
            senderPane->setActiveChildUrl(item.url());
        }

        Q_EMIT urlChanged(m_columns.at(colIndex)->dirUrl());
    }

    senderPane->controller()->selectionManager()->blockSignals(false);
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

    // The base write-state tracking is driven by the (unused) base model, so
    // recompute it from the active pane now that a folder has finished loading.
    // Otherwise "Create New"/paste stay disabled as if the folder were read-only.
    updateWritableState();

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

QUrl DolphinColumnsView::folderUrlForItem(const KFileItem &item) const
{
    if (item.isNull()) {
        return QUrl();
    }
    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (!folderUrl.isEmpty()) {
        return folderUrl;
    }
    return item.isDir() ? item.url() : QUrl();
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
    // If the child column already shows this folder, keep it. A single
    // interaction can request opening the same sub-folder from more than one
    // handler (currentItemChanged and selectionChanged), which would otherwise
    // tear down and rebuild the child pane, making it flicker/appear twice.
    if (columnIndex + 1 < m_columns.size() && m_columns.at(columnIndex + 1)->dirUrl() == childUrl) {
        return;
    }

    popAfter(columnIndex);

    // Mark the parent's selected item; guard against cascading navigation
    // since setActiveChildUrl triggers slotCurrentChanged in the pane.
    auto *parentPane = m_columns.at(columnIndex);
    {
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        parentPane->setActiveChildUrl(childUrl);
    }

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
    pane->setZoomLevel(zoomLevel());
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
        }
    });
    connect(pane, &DolphinColumnPane::currentItemChanged, this, &DolphinColumnsView::slotColumnsCurrentItemChanged);
    connect(pane, &DolphinColumnPane::directoryLoadingCompleted, this, &DolphinColumnsView::slotPaneLoadingCompleted);

    // In "adjust to content" mode a rename or an added/removed item changes how wide
    // the column needs to be. A KIO rename refreshes the item in place (itemsChanged
    // with the "text" role); a rename seen on disk arrives as a remove plus insert.
    // Handle both so the column re-fits either way.
    connect(model, &KFileItemModel::itemsInserted, this, &DolphinColumnsView::refitColumnsToContent);
    connect(model, &KFileItemModel::itemsRemoved, this, &DolphinColumnsView::refitColumnsToContent);
    connect(model, &KFileItemModel::itemsChanged, this, [this](const KItemRangeList &, const QSet<QByteArray> &changedRoles) {
        if (changedRoles.contains("text")) {
            refitColumnsToContent();
        }
    });

    // Forward VCS observer messages
    connect(pane, &DolphinColumnPane::infoMessage, this, &DolphinView::infoMessage);
    connect(pane, &DolphinColumnPane::errorMessage, this, [this](const QString &msg) {
        Q_EMIT errorMessage(msg, KIO::ERR_UNKNOWN);
    });
    connect(pane, &DolphinColumnPane::operationCompletedMessage, this, &DolphinView::operationCompletedMessage);

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

    // Drop handling: reuse base class helper
    auto controller = pane->controller();
    connect(controller, &KItemListController::itemDropEvent, this, [this, pane](int index, QGraphicsSceneDragDropEvent *event) {
        handleItemDropEvent(pane->model(), pane->dirUrl(), index, event);
    });

    connect(controller, &KItemListController::mouseButtonPressed, this, [this, pane](int itemIndex, Qt::MouseButtons buttons) {
        handleMouseButtonPressed(pane, itemIndex, buttons);
    });

    connect(controller, &KItemListController::itemHovered, this, [this, pane, controller](int index) {
        const KFileItem item = pane->model()->fileItem(index);
        if (GeneralSettings::showToolTips() && !isDragging()) {
            QRectF itemRect = controller->view()->itemContextRect(index);
            const QPoint pos = pane->container()->mapToGlobal(itemRect.topLeft().toPoint());
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

    return pane;
}

bool DolphinColumnsView::eventFilter(QObject *watched, QEvent *event)
{
    // Double-clicking a splitter handle fits all columns to their content.
    if (event->type() == QEvent::MouseButtonDblClick) {
        for (int i = 1; i < m_splitter->count(); ++i) {
            if (m_splitter->handle(i) == watched) {
                autoAdjustColumns();
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
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        return DolphinView::eventFilter(watched, event);
    }

    // A pane gaining focus makes both the view and that column active, mirroring
    // how the base DolphinView activates on FocusIn of its container. Focus is
    // how the window learns which view is active and wires the active-view
    // signals (context menu, selection info, ...) to it, and the active-pane
    // context-menu handler follows the active column. Activating the column here
    // also covers a right-click on an inactive pane's background (which selects
    // no item, so handleMouseButtonPressed() does not run). Doing this on FocusIn
    // covers mouse, keyboard and programmatic focus, and it is reinstalled per
    // pane in createPane(), so it is unaffected by swapView().
    if (event->type() == QEvent::FocusIn) {
        if (sourceColumn != m_activeColumn) {
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
        return;
    }

    // No child column yet, try to open the current item as one
    auto *pane = m_columns.at(sourceColumn);
    const int current = pane->controller()->selectionManager()->currentItem();
    if (current >= 0 && current < pane->model()->count()) {
        const KFileItem item = pane->model()->fileItem(current);
        if (!item.isNull()) {
            const QUrl folderUrl = folderUrlForItem(item);
            if (!folderUrl.isEmpty()) {
                openChild(sourceColumn, folderUrl);
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

    const QUrl targetUrl = folderUrlForItem(item);
    if (targetUrl.isEmpty()) {
        Q_EMIT itemActivated(item);
        return true;
    }

    const int childCol = sourceColumn + 1;
    // Reopen the child column only when it is not already showing this folder,
    // so stepping into an already-open child does not tear it down and reload.
    if (childCol >= m_columns.size() || m_columns.at(childCol)->dirUrl() != targetUrl) {
        openChild(sourceColumn, targetUrl);
    }

    // Step into the child column, whether it was just opened or already present.
    if (childCol < m_columns.size()) {
        if (m_columns.at(childCol)->model()->count() > 0) {
            // Keep an existing selection in the already-open child untouched.
            if (!m_columns.at(childCol)->controller()->selectionManager()->hasSelection()) {
                autoSelectFirstItem(childCol);
            }
        } else {
            m_pendingAutoSelect = m_columns.at(childCol);
        }
        setActiveColumn(childCol);
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

    const QUrl targetUrl = folderUrlForItem(item);
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
        QScopedValueRollback<bool> navigationGuard(m_blockNavigation, true);
        if (selectionManager->currentItem() < 0) {
            selectionManager->setCurrentItem(0);
        }
        // Always re-apply selection; it may have been cleared by Left arrow
        selectionManager->setSelected(selectionManager->currentItem(), 1, KItemListSelectionManager::Select);
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

    const QUrl folderUrl = folderUrlForItem(item);
    if (!folderUrl.isEmpty()) {
        openChild(columnIndex, folderUrl);
    }
}

void DolphinColumnsView::refitColumnsToContent()
{
    if (ColumnsModeSettings::self()->dynamicColumnWidth()) {
        recalculateColumnWidths();
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
    const int handleWidth = m_splitter->handleWidth();
    // Reserve room for the handles between visible columns so a full set of
    // default-width columns fits the viewport exactly, instead of overflowing it
    // by a few pixels and tripping the horizontal scrollbar.
    const int availableForColumns = qMax(0, viewportWidth - (divisor - 1) * handleWidth);
    const int minColumnWidth = ColumnsModeSettings::self()->minColumnWidth();
    const int defaultWidth = qMax(minColumnWidth, availableForColumns / divisor);
    const bool dynamicWidth = ColumnsModeSettings::self()->dynamicColumnWidth();

    QList<int> sizes;
    sizes.reserve(m_splitter->count());
    for (int i = 0; i < numColumns; ++i) {
        if (m_customColumnWidths.contains(i)) {
            // A width the user set by dragging the handle always wins.
            sizes.append(m_customColumnWidths.value(i));
        } else if (dynamicWidth) {
            // Size the column to its content, never below the configured minimum.
            sizes.append(qMax(minColumnWidth, m_columns.at(i)->calculateOptimalWidth()));
        } else {
            sizes.append(defaultWidth);
        }
    }
    applyColumnSizes(sizes);
}

void DolphinColumnsView::applyColumnSizes(QList<int> columnSizes)
{
    const int numColumns = m_columns.size();
    const int handleWidth = m_splitter->handleWidth();

    columnSizes.append(0); // filler
    m_splitter->setSizes(columnSizes);

    int totalWidth = 0;
    for (int i = 0; i < numColumns; ++i) {
        totalWidth += columnSizes.at(i);
    }
    totalWidth += (numColumns - 1) * handleWidth;

    // Tolerate one handle of overshoot so a one-pixel wobble of the viewport
    // width under fractional scaling does not flip the scrollbar on and off
    // while resizing.
    const int viewportWidth = m_scrollArea ? m_scrollArea->viewport()->width() : width();
    if (totalWidth > viewportWidth + handleWidth) {
        m_splitter->setMinimumWidth(totalWidth);
    } else {
        m_splitter->setMinimumWidth(0);
    }
}

void DolphinColumnsView::autoAdjustColumns()
{
    if (m_columns.isEmpty()) {
        return;
    }

    // Fit every column to its content and drop any width the user dragged, so the
    // whole set is tidy again.
    m_customColumnWidths.clear();
    const int minColumnWidth = ColumnsModeSettings::self()->minColumnWidth();
    const bool fixedWidth = !ColumnsModeSettings::self()->dynamicColumnWidth();

    QList<int> sizes;
    sizes.reserve(m_columns.size());
    for (int i = 0; i < m_columns.size(); ++i) {
        const int width = qMax(minColumnWidth, m_columns.at(i)->calculateOptimalWidth());
        sizes.append(width);
        if (fixedWidth) {
            // In fixed-width mode the fit is a one-off, so pin it as a custom width.
            // Otherwise the next relayout (e.g. a resize) would snap the column
            // back to the fixed width. In adjust-to-content mode the layout already
            // tracks the content, so no pinning is needed.
            m_customColumnWidths[i] = width;
        }
    }
    applyColumnSizes(sizes);
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
        // Programmatic selection changes (e.g. openChild() marking the parent's
        // active child) must not drive navigation, or they re-enter openChild()
        // and build the child column twice. slotColumnsCurrentItemChanged()
        // guards the same way.
        if (m_blockNavigation) {
            return;
        }
        const bool stateChanged = (current.isEmpty() != previous.isEmpty());
        m_columnsSelectionTimer->setInterval(stateChanged ? 0 : 300);
        m_columnsSelectionTimer->start();

        if (current.count() != 1) {
            return;
        }

        auto currentColumnIndex = m_columns.indexOf(newPane);
        auto currentUrl = newPane->dirUrl();
        auto item = newPane->model()->fileItem(current.first());
        if (item.isDir() && currentColumnIndex != -1) {
            const int childCol = currentColumnIndex + 1;
            // The child column may already show this folder because
            // slotColumnsCurrentItemChanged() opened it for the same change.
            // Only pop the stale descendant columns and (re)open it otherwise,
            // so the child pane is not rebuilt twice for one interaction.
            if (childCol >= m_columns.size() || m_columns.at(childCol)->dirUrl() != item.url()) {
                for (int i = m_columns.size() - 1; i > 0; --i) {
                    if (currentUrl.isParentOf(m_columns.at(i)->dirUrl())) {
                        popAfter(i - 1);
                    } else {
                        break;
                    }
                }

                newPane->controller()->selectionManager()->blockSignals(true);
                openChild(currentColumnIndex, item.url());

                updateUrl(m_columns.at(m_activeColumn)->dirUrl());
                Q_EMIT urlChanged(url());

                newPane->controller()->selectionManager()->blockSignals(false);
            }
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
