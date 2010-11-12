/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2008 by Simon St. James (kdedevel@etotheipiplusone.com) *
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

#include "dolphindetailsview.h"

#include "additionalinfoaccessor.h"
#include "dolphinmodel.h"
#include "dolphinviewcontroller.h"
#include "dolphinfileitemdelegate.h"
#include "settings/dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinviewautoscroller.h"
#include "draganddrophelper.h"
#include "viewextensionsfactory.h"
#include "viewmodecontroller.h"
#include "viewproperties.h"
#include "zoomlevelinfo.h"

#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"

#include <kdirmodel.h>
#include <kdirlister.h>
#include <klocale.h>
#include <kmenu.h>

#include <QApplication>
#include <QHeaderView>
#include <QScrollBar>

DolphinDetailsView::DolphinDetailsView(QWidget* parent,
                                       DolphinViewController* dolphinViewController,
                                       const ViewModeController* viewModeController,
                                       DolphinSortFilterProxyModel* proxyModel) :
    DolphinTreeView(parent),
    m_autoResize(true),
    m_dolphinViewController(dolphinViewController),
    m_viewModeController(viewModeController),
    m_extensionsFactory(0),
    m_expandableFoldersAction(0),
    m_expandedUrls(),
    m_font(),
    m_decorationSize()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    Q_ASSERT(dolphinViewController != 0);
    Q_ASSERT(viewModeController != 0);

    setLayoutDirection(Qt::LeftToRight);
    setAcceptDrops(true);
    setSortingEnabled(true);
    setUniformRowHeights(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setAlternatingRowColors(true);
    setRootIsDecorated(settings->expandableFolders());
    setItemsExpandable(settings->expandableFolders());
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setModel(proxyModel);

    setMouseTracking(true);

    const ViewProperties props(viewModeController->url());
    setSortIndicatorSection(props.sorting());
    setSortIndicatorOrder(props.sortOrder());

    QHeaderView* headerView = header();
    connect(headerView, SIGNAL(sectionClicked(int)),
            this, SLOT(synchronizeSortingState(int)));
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(configureSettings(const QPoint&)));
    connect(headerView, SIGNAL(sectionResized(int, int, int)),
            this, SLOT(slotHeaderSectionResized(int, int, int)));
    connect(headerView, SIGNAL(sectionHandleDoubleClicked(int)),
            this, SLOT(disableAutoResizing()));

    connect(parent, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(setSortIndicatorSection(DolphinView::Sorting)));
    connect(parent, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(setSortIndicatorOrder(Qt::SortOrder)));

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            dolphinViewController, SLOT(requestTab(const QModelIndex&)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    }

    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            dolphinViewController, SLOT(emitViewportEntered()));
    connect(viewModeController, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(setZoomLevel(int)));
    connect(dolphinViewController->view(), SIGNAL(additionalInfoChanged()),
            this, SLOT(updateColumnVisibility()));
    connect(viewModeController, SIGNAL(activationChanged(bool)),
            this, SLOT(slotActivationChanged(bool)));

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       qRound(settings->fontSize()),
                       settings->fontWeight(),
                       settings->italicFont());
        m_font.setPointSizeF(settings->fontSize());
    }

    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    setHorizontalScrollMode(QTreeView::ScrollPerPixel);

    const DolphinView* view = dolphinViewController->view();
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));


    setFocus();
    viewport()->installEventFilter(this);

    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
            this, SLOT(slotGlobalSettingsChanged(int)));

    m_expandableFoldersAction = new QAction(i18nc("@option:check", "Expandable Folders"), this);
    m_expandableFoldersAction->setCheckable(true);
    connect(m_expandableFoldersAction, SIGNAL(toggled(bool)),
            this, SLOT(setFoldersExpandable(bool)));

    connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(slotExpanded(const QModelIndex&)));
    connect(this, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(slotCollapsed(const QModelIndex&)));

    updateDecorationSize(view->showPreview());

    m_extensionsFactory = new ViewExtensionsFactory(this, dolphinViewController, viewModeController);
    m_extensionsFactory->fileItemDelegate()->setMinimizedNameColumn(true);

    KDirLister *dirLister = qobject_cast<KDirModel*>(proxyModel->sourceModel())->dirLister();
    connect(dirLister, SIGNAL(newItems(KFileItemList)), this, SLOT(resizeColumns()));
}

DolphinDetailsView::~DolphinDetailsView()
{
}

QSet<KUrl> DolphinDetailsView::expandedUrls() const
{
    return m_expandedUrls;
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        header()->setResizeMode(QHeaderView::Interactive);
        updateColumnVisibility();
    }

    return DolphinTreeView::event(event);
}

QStyleOptionViewItem DolphinDetailsView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = DolphinTreeView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.fontMetrics = QFontMetrics(m_font);
    viewOptions.showDecorationSelected = true;
    viewOptions.decorationSize = m_decorationSize;
    return viewOptions;
}

void DolphinDetailsView::contextMenuEvent(QContextMenuEvent* event)
{
    DolphinTreeView::contextMenuEvent(event);

    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    m_expandableFoldersAction->setChecked(settings->expandableFolders());
    m_dolphinViewController->triggerContextMenuRequest(event->pos(),
                                            QList<QAction*>() << m_expandableFoldersAction);
}

void DolphinDetailsView::mousePressEvent(QMouseEvent* event)
{
    m_dolphinViewController->requestActivation();

    DolphinTreeView::mousePressEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        // The mouse press is done somewhere outside the filename column
        if (QApplication::mouseButtons() & Qt::MidButton) {
            m_dolphinViewController->replaceUrlByClipboard();
        }
    }
}

void DolphinDetailsView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions, m_dolphinViewController);
    DolphinTreeView::startDrag(supportedActions);
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        event->acceptProposedAction();
    }
    DolphinTreeView::dragEnterEvent(event);
}

void DolphinDetailsView::dragMoveEvent(QDragMoveEvent* event)
{
    DolphinTreeView::dragMoveEvent(event);

    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        // Accept URL drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    KFileItem item;
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        item = m_dolphinViewController->itemForIndex(index);
    }
    m_dolphinViewController->indicateDroppedUrls(item, m_viewModeController->url(), event);
    DolphinTreeView::dropEvent(event);
}

void DolphinDetailsView::keyPressEvent(QKeyEvent* event)
{
    DolphinTreeView::keyPressEvent(event);
    m_dolphinViewController->handleKeyPressEvent(event);
}

void DolphinDetailsView::resizeEvent(QResizeEvent* event)
{
    DolphinTreeView::resizeEvent(event);
    if (m_autoResize) {
        resizeColumns();
    }
}

void DolphinDetailsView::wheelEvent(QWheelEvent* event)
{
    const int step = m_decorationSize.height();
    verticalScrollBar()->setSingleStep(step);
    DolphinTreeView::wheelEvent(event);
}

void DolphinDetailsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    m_extensionsFactory->handleCurrentIndexChange(current, previous);
    DolphinTreeView::currentChanged(current, previous);

    // If folders are expanded, the width which is available for editing may have changed
    // because it depends on the level of the current item in the folder hierarchy.
    adjustMaximumSizeForEditing(current);
}

bool DolphinDetailsView::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == viewport()) && (event->type() == QEvent::Leave)) {
        // If the mouse is above an item and moved very fast outside the widget,
        // no viewportEntered() signal might be emitted although the mouse has been moved
        // above the viewport.
        m_dolphinViewController->emitViewportEntered();
    }

    return DolphinTreeView::eventFilter(watched, event);
}

QRect DolphinDetailsView::visualRect(const QModelIndex& index) const
{
    QRect rect = DolphinTreeView::visualRect(index);
    const KFileItem item = m_dolphinViewController->itemForIndex(index);
    if (!item.isNull()) {
        const int width = DolphinFileItemDelegate::nameColumnWidth(item.text(), viewOptions());
        rect.setWidth(width);
    }

    return rect;
}

bool DolphinDetailsView::acceptsDrop(const QModelIndex& index) const
{
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        // Accept drops above directories
        const KFileItem item = m_dolphinViewController->itemForIndex(index);
        return !item.isNull() && item.isDir();
    }

    return false;
}

void DolphinDetailsView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    removeExpandedIndexes(parent, start, end);
    DolphinTreeView::rowsAboutToBeRemoved(parent, start, end);
}

void DolphinDetailsView::setSortIndicatorSection(DolphinView::Sorting sorting)
{
    header()->setSortIndicator(sorting, header()->sortIndicatorOrder());
}

void DolphinDetailsView::setSortIndicatorOrder(Qt::SortOrder sortOrder)
{
    header()->setSortIndicator(header()->sortIndicatorSection(), sortOrder);
}

void DolphinDetailsView::synchronizeSortingState(int column)
{
    // The sorting has already been changed in QTreeView if this slot is
    // invoked, but Dolphin is not informed about this.
    DolphinView::Sorting sorting = DolphinSortFilterProxyModel::sortingForColumn(column);
    const Qt::SortOrder sortOrder = header()->sortIndicatorOrder();
    m_dolphinViewController->indicateSortingChange(sorting);
    m_dolphinViewController->indicateSortOrderChange(sortOrder);
}

void DolphinDetailsView::slotEntered(const QModelIndex& index)
{
    if (index.column() == DolphinModel::Name) {
        m_dolphinViewController->emitItemEntered(index);
    } else {
        m_dolphinViewController->emitViewportEntered();
    }
}

void DolphinDetailsView::setZoomLevel(int level)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(level);
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();

    const bool showPreview = m_dolphinViewController->view()->showPreview();
    if (showPreview) {
        settings->setPreviewSize(size);
    } else {
        settings->setIconSize(size);
    }

    updateDecorationSize(showPreview);
}

void DolphinDetailsView::slotShowPreviewChanged()
{
    const DolphinView* view = m_dolphinViewController->view();
    updateDecorationSize(view->showPreview());
}

void DolphinDetailsView::configureSettings(const QPoint& pos)
{
    KMenu popup(this);
    popup.addTitle(i18nc("@title:menu", "Columns"));

    // Add checkbox items for each column
    QHeaderView* headerView = header();
    const int columns = model()->columnCount();
    for (int i = 0; i < columns; ++i) {
        const int logicalIndex = headerView->logicalIndex(i);
        const QString text = model()->headerData(logicalIndex, Qt::Horizontal).toString();
        if (!text.isEmpty()) {
            QAction* action = popup.addAction(text);
            action->setCheckable(true);
            action->setChecked(!headerView->isSectionHidden(logicalIndex));
            action->setData(logicalIndex);
            action->setEnabled(logicalIndex != DolphinModel::Name);
        }
    }
    popup.addSeparator();

    QAction* activatedAction = popup.exec(header()->mapToGlobal(pos));
    if (activatedAction != 0) {
        const bool show = activatedAction->isChecked();
        const int columnIndex = activatedAction->data().toInt();

        KFileItemDelegate::InformationList list = m_dolphinViewController->view()->additionalInfo();
        const KFileItemDelegate::Information info = infoForColumn(columnIndex);
        if (show) {
            Q_ASSERT(!list.contains(info));
            list.append(info);
        } else {
            Q_ASSERT(list.contains(info));
            const int index = list.indexOf(info);
            list.removeAt(index);
        }

        m_dolphinViewController->indicateAdditionalInfoChange(list);
        setColumnHidden(columnIndex, !show);
        resizeColumns();
    }
}

void DolphinDetailsView::updateColumnVisibility()
{
    QHeaderView* headerView = header();
    disconnect(headerView, SIGNAL(sectionMoved(int, int, int)),
               this, SLOT(saveColumnPositions()));

    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    const QList<int> columnPositions = settings->columnPositions();

    const KFileItemDelegate::InformationList list = m_dolphinViewController->view()->additionalInfo();
    for (int i = DolphinModel::Name; i < DolphinModel::ExtraColumnCount; ++i) {
        const KFileItemDelegate::Information info = infoForColumn(i);
        const bool hide = !list.contains(info) && (i != DolphinModel::Name);
        if (isColumnHidden(i) != hide) {
            setColumnHidden(i, hide);
        }

        // If the list columnPositions has been written by an older Dolphin version,
        // its length might be smaller than DolphinModel::ExtraColumnCount. Therefore,
        // we have to check if item number i exists before accessing it.
        if (i < columnPositions.length()) {
            const int position = columnPositions[i];

            // The position might be outside the correct range if the list columnPositions
            // has been written by a newer Dolphin version with more columns.
            if (position < DolphinModel::ExtraColumnCount) {
                const int from = headerView->visualIndex(i);
                headerView->moveSection(from, position);
            }
        }
    }

    resizeColumns();

    connect(headerView, SIGNAL(sectionMoved(int, int, int)),
            this, SLOT(saveColumnPositions()));
}

void DolphinDetailsView::resizeColumns()
{
    // Using the resize mode QHeaderView::ResizeToContents is too slow (it takes
    // around 3 seconds for each (!) resize operation when having > 10000 items).
    // This gets a problem especially when opening large directories, where several
    // resize operations are received for showing the currently available items during
    // loading (the application hangs around 20 seconds when loading > 10000 items).

    QHeaderView* headerView = header();
    const int rowCount = model()->rowCount();
    QFontMetrics fontMetrics(viewport()->font());

    // Define the maximum number of rows, where an exact (but expensive) calculation
    // of the widths is done.
    const int maxRowCount = 200;

    // Calculate the required with for each column and store it in columnWidth[]
    int columnWidth[DolphinModel::ExtraColumnCount];

    for (int column = 0; column < DolphinModel::ExtraColumnCount; ++column) {
        columnWidth[column] = 0;
        if (!isColumnHidden(column)) {
            // Calculate the required width for the current column and consider only
            // up to maxRowCount columns for performance reasons
            if (rowCount > 0) {
                const QAbstractProxyModel* proxyModel = qobject_cast<const QAbstractProxyModel*>(model());
                const KDirModel* dirModel = qobject_cast<const KDirModel*>(proxyModel->sourceModel());

                const int count = qMin(rowCount, maxRowCount);
                const QStyleOptionViewItem option = viewOptions();
                for (int row = 0; row < count; ++row) {
                    const QModelIndex index = dirModel->index(row, column);
                    const int width = itemDelegate()->sizeHint(option, index).width();
                    if (width > columnWidth[column]) {
                        columnWidth[column] = width;
                    }
                }
            }

            // Assure that the required width is sufficient for the header too
            const int logicalIndex = headerView->logicalIndex(column);
            const QString headline = model()->headerData(logicalIndex, Qt::Horizontal).toString();
            // TODO: check Qt-sources which left/right-gap is used for the headlines
            const int headlineWidth = fontMetrics.width(headline) + 20;

            columnWidth[column] = qMax(columnWidth[column], headlineWidth);
        }
    }

    // Resize all columns except of the name column
    int requiredWidth = 0;
    for (int column = KDirModel::Size; column < DolphinModel::ExtraColumnCount; ++column) {
        if (!isColumnHidden(column)) {
            requiredWidth += columnWidth[column];
            headerView->resizeSection(column, columnWidth[column]);
        }
    }

    // Resize the name column in a way that the whole available width is used
    columnWidth[KDirModel::Name] = viewport()->width() - requiredWidth;

    const int minNameWidth = 300;
    if (columnWidth[KDirModel::Name] < minNameWidth) {
        columnWidth[KDirModel::Name] = minNameWidth;

        if ((rowCount > 0) && (rowCount < maxRowCount)) {
            // Try to decrease the name column width without clipping any text
            const int nameWidth = sizeHintForColumn(DolphinModel::Name);
            if (nameWidth + requiredWidth <= viewport()->width()) {
                columnWidth[KDirModel::Name] = viewport()->width() - requiredWidth;
            } else if (nameWidth < minNameWidth) {
                columnWidth[KDirModel::Name] = nameWidth;
            }
        }
    }

    headerView->resizeSection(KDirModel::Name, columnWidth[KDirModel::Name]);
}

void DolphinDetailsView::saveColumnPositions()
{
    QList<int> columnPositions;
    for (int i = DolphinModel::Name; i < DolphinModel::ExtraColumnCount; ++i) {
        columnPositions.append(header()->visualIndex(i));
    }

    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    settings->setColumnPositions(columnPositions);
}

void DolphinDetailsView::slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    // If the user changes the size of the headers, the autoresize feature should be
    // turned off. As there is no dedicated interface to find out whether the header
    // section has been resized by the user or by a resize event, another approach is used.
    // Attention: Take care when changing the if-condition to verify that there is no
    // regression in combination with bug 178630 (see fix in comment #8).
    if ((QApplication::mouseButtons() & Qt::LeftButton) && header()->underMouse()) {
        disableAutoResizing();
    }

    adjustMaximumSizeForEditing(currentIndex());
}

void DolphinDetailsView::slotActivationChanged(bool active)
{
    setAlternatingRowColors(active);
}

void DolphinDetailsView::disableAutoResizing()
{
    m_autoResize = false;
}

void DolphinDetailsView::requestActivation()
{
    m_dolphinViewController->requestActivation();
}

void DolphinDetailsView::slotGlobalSettingsChanged(int category)
{
    Q_UNUSED(category);

    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    }
    // Disconnect then reconnect, since the settings have been changed, the connection requirements may have also.
    disconnect(this, SIGNAL(clicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    disconnect(this, SIGNAL(doubleClicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    } else {
        connect(this, SIGNAL(doubleClicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    }
}


void DolphinDetailsView::setFoldersExpandable(bool expandable)
{
    if (!expandable) {
        // Collapse all expanded folders, as QTreeView::setItemsExpandable(false)
        // does not do this task
        const int rowCount = model()->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            setExpanded(model()->index(row, 0), false);
        }
    }
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    settings->setExpandableFolders(expandable);
    setRootIsDecorated(expandable);
    setItemsExpandable(expandable);

    // The width of the space which is available for editing has changed
    // because of the (dis)appearance of the expanding toggles
    adjustMaximumSizeForEditing(currentIndex());
}

void DolphinDetailsView::slotExpanded(const QModelIndex& index)
{
    KFileItem item = m_dolphinViewController->itemForIndex(index);
    if (!item.isNull()) {
        m_expandedUrls.insert(item.url());
    }
}

void DolphinDetailsView::slotCollapsed(const QModelIndex& index)
{
    KFileItem item = m_dolphinViewController->itemForIndex(index);
    if (!item.isNull()) {
        m_expandedUrls.remove(item.url());
    }
}

void DolphinDetailsView::removeExpandedIndexes(const QModelIndex& parent, int start, int end)
{
    if (m_expandedUrls.isEmpty()) {
        return;
    }

    for (int row = start; row <= end; row++) {
        const QModelIndex index = model()->index(row, 0, parent);
        if (isExpanded(index)) {
            slotCollapsed(index);
            removeExpandedIndexes(index, 0, model()->rowCount(index) - 1);
        }
    }
}

void DolphinDetailsView::updateDecorationSize(bool showPreview)
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    const int iconSize = showPreview ? settings->previewSize() : settings->iconSize();
    setIconSize(QSize(iconSize, iconSize));
    m_decorationSize = QSize(iconSize, iconSize);

    if (m_extensionsFactory) {
        // The old maximumSize used by KFileItemDelegate is not valid any more after the icon size change.
        // It must be discarded before doItemsLayout() is called (see bug 234600).
        m_extensionsFactory->fileItemDelegate()->setMaximumSize(QSize());
    }

    doItemsLayout();

    // Calculate the new maximumSize for KFileItemDelegate after the icon size change.
    QModelIndex current = currentIndex();
    if (current.isValid()) {
        adjustMaximumSizeForEditing(current);
    }
}

KFileItemDelegate::Information DolphinDetailsView::infoForColumn(int columnIndex) const
{
    return AdditionalInfoAccessor::instance().keyForColumn(columnIndex);
}

void DolphinDetailsView::adjustMaximumSizeForEditing(const QModelIndex& index)
{
    // Make sure that the full width of the "Name" column is available for "Rename Inline"
    m_extensionsFactory->fileItemDelegate()->setMaximumSize(QTreeView::visualRect(index).size());
}

#include "dolphindetailsview.moc"
