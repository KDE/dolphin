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

#include "dolphinmodel.h"
#include "dolphincontroller.h"
#include "dolphinfileitemdelegate.h"
#include "settings/dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinviewautoscroller.h"
#include "draganddrophelper.h"
#include "viewextensionsfactory.h"
#include "viewproperties.h"
#include "zoomlevelinfo.h"

#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"

#include <kdirmodel.h>
#include <klocale.h>
#include <kmenu.h>

#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QRubberBand>
#include <QPainter>
#include <QScrollBar>

DolphinDetailsView::DolphinDetailsView(QWidget* parent,
                                       DolphinController* controller,
                                       DolphinSortFilterProxyModel* proxyModel) :
    QTreeView(parent),
    m_autoResize(true),
    m_expandingTogglePressed(false),
    m_keyPressed(false),
    m_useDefaultIndexAt(true),
    m_ignoreScrollTo(false),
    m_controller(controller),
    m_extensionsFactory(0),
    m_expandableFoldersAction(0),
    m_expandedUrls(),
    m_font(),
    m_decorationSize(),
    m_band()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    Q_ASSERT(controller != 0);

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

    const ViewProperties props(controller->url());
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
            controller, SLOT(requestTab(const QModelIndex&)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    }

    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(setZoomLevel(int)));
    connect(controller->dolphinView(), SIGNAL(additionalInfoChanged()),
            this, SLOT(updateColumnVisibility()));
    connect(controller, SIGNAL(activationChanged(bool)),
            this, SLOT(slotActivationChanged(bool)));

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       settings->fontSize(),
                       settings->fontWeight(),
                       settings->italicFont());
    }

    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    setHorizontalScrollMode(QTreeView::ScrollPerPixel);

    const DolphinView* view = controller->dolphinView();
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));


    setFocus();
    viewport()->installEventFilter(this);

    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
            this, SLOT(slotGlobalSettingsChanged(int)));

    m_useDefaultIndexAt = false;

    m_expandableFoldersAction = new QAction(i18nc("@option:check", "Expandable Folders"), this);
    m_expandableFoldersAction->setCheckable(true);
    connect(m_expandableFoldersAction, SIGNAL(toggled(bool)),
            this, SLOT(setFoldersExpandable(bool)));

    connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(slotExpanded(const QModelIndex&)));
    connect(this, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(slotCollapsed(const QModelIndex&)));

    updateDecorationSize(view->showPreview());

    m_extensionsFactory = new ViewExtensionsFactory(this, controller);
    m_extensionsFactory->fileItemDelegate()->setMinimizedNameColumn(true);
    m_extensionsFactory->setAutoFolderExpandingEnabled(settings->expandableFolders());
}

DolphinDetailsView::~DolphinDetailsView()
{
}

QSet<KUrl> DolphinDetailsView::expandedUrls() const
{
    return m_expandedUrls;
}

QRegion DolphinDetailsView::visualRegionForSelection(const QItemSelection &selection) const
{
    // We have to make sure that the visualRect of each model index is inside the region.
    // QTreeView::visualRegionForSelection does not do it right because it assumes implicitly
    // that all visualRects have the same width, which is in general not the case here.
    QRegion selectionRegion;
    const QModelIndexList indexes = selection.indexes();

    foreach(const QModelIndex& index, indexes) {
        selectionRegion += visualRect(index);
    }

    return selectionRegion;
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        header()->setResizeMode(QHeaderView::Interactive);
        updateColumnVisibility();
    }

    return QTreeView::event(event);
}

QStyleOptionViewItem DolphinDetailsView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = QTreeView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.showDecorationSelected = true;
    viewOptions.decorationSize = m_decorationSize;
    return viewOptions;
}

void DolphinDetailsView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);

    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    m_expandableFoldersAction->setChecked(settings->expandableFolders());
    m_controller->triggerContextMenuRequest(event->pos(),
                                            QList<QAction*>() << m_expandableFoldersAction);
}

void DolphinDetailsView::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();

    const QModelIndex current = currentIndex();
    QTreeView::mousePressEvent(event);

    m_expandingTogglePressed = isAboveExpandingToggle(event->pos());

    const QModelIndex index = indexAt(event->pos());
    const bool updateState = index.isValid() &&
                             (index.column() == DolphinModel::Name) &&
                             (event->button() == Qt::LeftButton);
    if (updateState) {
        setState(QAbstractItemView::DraggingState);
    }

    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        // the mouse press is done somewhere outside the filename column
        if (QApplication::mouseButtons() & Qt::MidButton) {
            m_controller->replaceUrlByClipboard();
        }

        const Qt::KeyboardModifiers mod = QApplication::keyboardModifiers();
        if (!m_expandingTogglePressed && !(mod & Qt::ShiftModifier) && !(mod & Qt::ControlModifier)) {
            clearSelection();
        }

        // restore the current index, other columns are handled as viewport area.
        // setCurrentIndex(...) implicitly calls scrollTo(...), which we want to ignore.
        m_ignoreScrollTo = true;
        selectionModel()->setCurrentIndex(current, QItemSelectionModel::Current);
        m_ignoreScrollTo = false;

        if ((event->button() == Qt::LeftButton) && !m_expandingTogglePressed) {
            // Inform Qt about what we are doing - otherwise it starts dragging items around!
            setState(DragSelectingState);
            m_band.show = true;
            // Incremental update data will not be useful - start from scratch.
            m_band.ignoreOldInfo = true;
            const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
            m_band.origin = event->pos()  + scrollPos;
            m_band.destination = m_band.origin;
            m_band.originalSelection = selectionModel()->selection();
        }
    }
}

void DolphinDetailsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_expandingTogglePressed) {
        // Per default QTreeView starts either a selection or a drag operation when dragging
        // the expanding toggle button (Qt-issue - see TODO comment in DolphinIconsView::mousePressEvent()).
        // Turn off this behavior in Dolphin to stay predictable:
        setState(QAbstractItemView::NoState);
        return;
    }

    if (m_band.show) {
        const QPoint mousePos = event->pos();
        const QModelIndex index = indexAt(mousePos);
        if (!index.isValid()) {
            // the destination of the selection rectangle is above the viewport. In this
            // case QTreeView does no selection at all, which is not the wanted behavior
            // in Dolphin -> select all items within the elastic band rectangle
            updateElasticBandSelection();
        }

        // TODO: enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
        updateElasticBand();
    } else {
        // TODO: enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
    }
}

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_expandingTogglePressed) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && (index.column() == DolphinModel::Name)) {
            QTreeView::mouseReleaseEvent(event);
        } else {
            // don't change the current index if the cursor is released
            // above any other column than the name column, as the other
            // columns act as viewport
            const QModelIndex current = currentIndex();
            QTreeView::mouseReleaseEvent(event);
            selectionModel()->setCurrentIndex(current, QItemSelectionModel::Current);
        }
    }
    m_expandingTogglePressed = false;

    if (m_band.show) {
        setState(NoState);
        updateElasticBand();
        m_band.show = false;
    }
}

void DolphinDetailsView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions, m_controller);
    m_band.show = false;
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        event->acceptProposedAction();
    }

    if (m_band.show) {
        updateElasticBand();
        m_band.show = false;
    }
}

void DolphinDetailsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void DolphinDetailsView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    setDirtyRegion(m_dropRect);
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        const KFileItem item = m_controller->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
        } else {
            m_dropRect.setSize(QSize()); // set as invalid
        }
        setDirtyRegion(m_dropRect);
    }

    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        // accept url drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    KFileItem item;
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        item = m_controller->itemForIndex(index);
    }
    m_controller->indicateDroppedUrls(item, m_controller->url(), event);
    QTreeView::dropEvent(event);
}

void DolphinDetailsView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);
    if (m_band.show) {
        // The following code has been taken from QListView
        // and adapted to DolphinDetailsView.
        // (C) 1992-2007 Trolltech ASA
        QStyleOptionRubberBand opt;
        opt.initFrom(this);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = elasticBandRect();

        QPainter painter(viewport());
        painter.save();
        style()->drawControl(QStyle::CE_RubberBand, &opt, &painter);
        painter.restore();
    }
}

void DolphinDetailsView::keyPressEvent(QKeyEvent* event)
{
    // If the Control modifier is pressed, a multiple selection
    // is done and DolphinDetailsView::currentChanged() may not
    // not change the selection in a custom way.
    m_keyPressed = !(event->modifiers() & Qt::ControlModifier);

    QTreeView::keyPressEvent(event);
    m_controller->handleKeyPressEvent(event);
}

void DolphinDetailsView::keyReleaseEvent(QKeyEvent* event)
{
    QTreeView::keyReleaseEvent(event);
    m_keyPressed = false;
}

void DolphinDetailsView::resizeEvent(QResizeEvent* event)
{
    QTreeView::resizeEvent(event);
    if (m_autoResize) {
        resizeColumns();
    }
}

void DolphinDetailsView::wheelEvent(QWheelEvent* event)
{
    const int height = m_decorationSize.height();
    const int step = (height >= KIconLoader::SizeHuge) ? height / 10 : (KIconLoader::SizeHuge - height) / 2;
    verticalScrollBar()->setSingleStep(step);
    QTreeView::wheelEvent(event);
}

void DolphinDetailsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTreeView::currentChanged(current, previous);
    m_extensionsFactory->handleCurrentIndexChange(current, previous);

    // Stay consistent with QListView: When changing the current index by key presses,
    // also change the selection.
    if (m_keyPressed) {
        setCurrentIndex(current);
    }
}

bool DolphinDetailsView::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == viewport()) && (event->type() == QEvent::Leave)) {
        // if the mouse is above an item and moved very fast outside the widget,
        // no viewportEntered() signal might be emitted although the mouse has been moved
        // above the viewport
        m_controller->emitViewportEntered();
    }

    return QTreeView::eventFilter(watched, event);
}

QModelIndex DolphinDetailsView::indexAt(const QPoint& point) const
{
    // the blank portion of the name column counts as empty space
    const QModelIndex index = QTreeView::indexAt(point);
    const bool isAboveEmptySpace  = !m_useDefaultIndexAt &&
                                    (index.column() == KDirModel::Name) && !visualRect(index).contains(point);
    return isAboveEmptySpace ? QModelIndex() : index;
}

QRect DolphinDetailsView::visualRect(const QModelIndex& index) const
{
    QRect rect = QTreeView::visualRect(index);
    const KFileItem item = m_controller->itemForIndex(index);
    if (!item.isNull()) {
        const int width = DolphinFileItemDelegate::nameColumnWidth(item.text(), viewOptions());
        rect.setWidth(width);
    }

    return rect;
}

void DolphinDetailsView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
    // We must override setSelection() as Qt calls it internally and when this happens
    // we must ensure that the default indexAt() is used.
    if (!m_band.show) {
        m_useDefaultIndexAt = true;
        QTreeView::setSelection(rect, command);
        m_useDefaultIndexAt = false;
    } else {
        // Use our own elastic band selection algorithm
        updateElasticBandSelection();
    }
}

void DolphinDetailsView::scrollTo(const QModelIndex & index, ScrollHint hint)
{
    if (!m_ignoreScrollTo) {
        QTreeView::scrollTo(index, hint);
    }
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
    m_controller->indicateSortingChange(sorting);
    m_controller->indicateSortOrderChange(sortOrder);
}

void DolphinDetailsView::slotEntered(const QModelIndex& index)
{
    if (index.column() == DolphinModel::Name) {
        m_controller->emitItemEntered(index);
    } else {
        m_controller->emitViewportEntered();
    }
}

void DolphinDetailsView::updateElasticBand()
{
    if (m_band.show) {
        QRect dirtyRegion(elasticBandRect());
        const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
        m_band.destination = viewport()->mapFromGlobal(QCursor::pos()) + scrollPos;
        // Going above the (logical) top-left of the view causes complications during selection;
        // we may as well prevent it.
        if (m_band.destination.y() < 0) {
            m_band.destination.setY(0);
        }
        if (m_band.destination.x() < 0) {
            m_band.destination.setX(0);
        }
        dirtyRegion = dirtyRegion.united(elasticBandRect());
        setDirtyRegion(dirtyRegion);
    }
}

QRect DolphinDetailsView::elasticBandRect() const
{
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());

    const QPoint topLeft = m_band.origin - scrollPos;
    const QPoint bottomRight = m_band.destination - scrollPos;
    return QRect(topLeft, bottomRight).normalized();
}

void DolphinDetailsView::setZoomLevel(int level)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(level);
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();

    const bool showPreview = m_controller->dolphinView()->showPreview();
    if (showPreview) {
        settings->setPreviewSize(size);
    } else {
        settings->setIconSize(size);
    }

    updateDecorationSize(showPreview);
}

void DolphinDetailsView::slotShowPreviewChanged()
{
    const DolphinView* view = m_controller->dolphinView();
    updateDecorationSize(view->showPreview());
}

void DolphinDetailsView::configureSettings(const QPoint& pos)
{
    KMenu popup(this);
    popup.addTitle(i18nc("@title:menu", "Columns"));

    // add checkbox items for each column
    QHeaderView* headerView = header();
    const int columns = model()->columnCount();
    for (int i = 0; i < columns; ++i) {
        const int logicalIndex = headerView->logicalIndex(i);
        const QString text = model()->headerData(logicalIndex, Qt::Horizontal).toString();
        QAction* action = popup.addAction(text);
        action->setCheckable(true);
        action->setChecked(!headerView->isSectionHidden(logicalIndex));
        action->setData(logicalIndex);
        action->setEnabled(logicalIndex != DolphinModel::Name);
    }
    popup.addSeparator();

    QAction* activatedAction = popup.exec(header()->mapToGlobal(pos));
    if (activatedAction != 0) {
        const bool show = activatedAction->isChecked();
        const int columnIndex = activatedAction->data().toInt();

        KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
        const KFileItemDelegate::Information info = infoForColumn(columnIndex);
        if (show) {
            Q_ASSERT(!list.contains(info));
            list.append(info);
        } else {
            Q_ASSERT(list.contains(info));
            const int index = list.indexOf(info);
            list.removeAt(index);
        }

        m_controller->indicateAdditionalInfoChange(list);
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
    
    const KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
    for (int i = DolphinModel::Name; i <= DolphinModel::Version; ++i) {
        const KFileItemDelegate::Information info = infoForColumn(i);
        const bool hide = !list.contains(info) && (i != DolphinModel::Name);
        if (isColumnHidden(i) != hide) {
            setColumnHidden(i, hide);
        }
        
        const int from = headerView->visualIndex(i);
        headerView->moveSection(from, columnPositions[i]);
    }
    
    resizeColumns();

    connect(headerView, SIGNAL(sectionMoved(int, int, int)),
            this, SLOT(saveColumnPositions()));

}

void DolphinDetailsView::saveColumnPositions()
{
    QList<int> columnPositions;
    for (int i = DolphinModel::Name; i <= DolphinModel::Version; ++i) {
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
    m_controller->requestActivation();
}

void DolphinDetailsView::slotGlobalSettingsChanged(int category)
{
    Q_UNUSED(category);

    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    }
    //Disconnect then reconnect, since the settings have been changed, the connection requirements may have also.
    disconnect(this, SIGNAL(clicked(QModelIndex)), m_controller, SLOT(triggerItem(QModelIndex)));
    disconnect(this, SIGNAL(doubleClicked(QModelIndex)), m_controller, SLOT(triggerItem(QModelIndex)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(QModelIndex)), m_controller, SLOT(triggerItem(QModelIndex)));
    } else {
        connect(this, SIGNAL(doubleClicked(QModelIndex)), m_controller, SLOT(triggerItem(QModelIndex)));
    }
}

void DolphinDetailsView::updateElasticBandSelection()
{
    if (!m_band.show) {
        return;
    }

    // Ensure the elastic band itself is up-to-date, in
    // case we are being called due to e.g. a drag event.
    updateElasticBand();

    // Clip horizontally to the name column, as some filenames will be
    // longer than the column.  We don't clip vertically as origin
    // may be above or below the current viewport area.
    const int nameColumnX = header()->sectionPosition(DolphinModel::Name);
    const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
    QRect selRect = elasticBandRect().normalized();
    QRect nameColumnArea(nameColumnX, selRect.y(), nameColumnWidth, selRect.height());
    selRect = nameColumnArea.intersect(selRect).normalized();
    // Get the last elastic band rectangle, expressed in viewpoint coordinates.
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QRect oldSelRect = QRect(m_band.lastSelectionOrigin - scrollPos, m_band.lastSelectionDestination - scrollPos).normalized();

    if (selRect.isNull()) {
        selectionModel()->select(m_band.originalSelection, QItemSelectionModel::ClearAndSelect);
        m_band.ignoreOldInfo = true;
        return;
    }

    if (!m_band.ignoreOldInfo) {
        // Do some quick checks to see if we can rule out the need to
        // update the selection.
        Q_ASSERT(uniformRowHeights());
        QModelIndex dummyIndex = model()->index(0, 0);
        if (!dummyIndex.isValid()) {
            // No items in the model presumably.
            return;
        }

        // If the elastic band does not cover the same rows as before, we'll
        // need to re-check, and also invalidate the old item distances.
        const int rowHeight = QTreeView::rowHeight(dummyIndex);
        const bool coveringSameRows =
            (selRect.top()    / rowHeight == oldSelRect.top()    / rowHeight) &&
            (selRect.bottom() / rowHeight == oldSelRect.bottom() / rowHeight);
        if (coveringSameRows) {
            // Covering the same rows, but have we moved far enough horizontally
            // that we might have (de)selected some other items?
            const bool itemSelectionChanged =
                ((selRect.left() > oldSelRect.left()) &&
                 (selRect.left() > m_band.insideNearestLeftEdge)) ||
                ((selRect.left() < oldSelRect.left()) &&
                 (selRect.left() <= m_band.outsideNearestLeftEdge)) ||
                ((selRect.right() < oldSelRect.right()) &&
                 (selRect.left() >= m_band.insideNearestRightEdge)) ||
                ((selRect.right() > oldSelRect.right()) &&
                 (selRect.right() >= m_band.outsideNearestRightEdge));

            if (!itemSelectionChanged) {
                return;
            }
        }
    }
    else {
        // This is the only piece of optimization data that needs to be explicitly
        // discarded.
        m_band.lastSelectionOrigin = QPoint();
        m_band.lastSelectionDestination = QPoint();
        oldSelRect = selRect;
    }

    // Do the selection from scratch. Force a update of the horizontal distances info.
    m_band.insideNearestLeftEdge   = nameColumnX + nameColumnWidth + 1;
    m_band.insideNearestRightEdge  = nameColumnX - 1;
    m_band.outsideNearestLeftEdge  = nameColumnX - 1;
    m_band.outsideNearestRightEdge = nameColumnX + nameColumnWidth + 1;

    // Include the old selection rect as well, so we can deselect
    // items that were inside it but not in the new selRect.
    const QRect boundingRect = selRect.united(oldSelRect).normalized();
    if (boundingRect.isNull()) {
        return;
    }

    // Get the index of the item in this row in the name column.
    // TODO - would this still work if the columns could be re-ordered?
    QModelIndex startIndex = QTreeView::indexAt(boundingRect.topLeft());
    if (startIndex.parent().isValid()) {
        startIndex = startIndex.parent().child(startIndex.row(), KDirModel::Name);
    } else {
        startIndex = model()->index(startIndex.row(), KDirModel::Name);
    }
    if (!startIndex.isValid()) {
        selectionModel()->select(m_band.originalSelection, QItemSelectionModel::ClearAndSelect);
        m_band.ignoreOldInfo = true;
        return;
    }

   // Go through all indexes between the top and bottom of boundingRect, and
   // update the selection.
   const int verticalCutoff = boundingRect.bottom();
   QModelIndex currIndex = startIndex;
   QModelIndex lastIndex;
   bool allItemsInBoundDone = false;

   // Calling selectionModel()->select(...) for each item that needs to be
   // toggled is slow as each call emits selectionChanged(...) so store them
   // and do the selection toggle in one batch.
   QItemSelection itemsToToggle;
   // QItemSelection's deal with continuous ranges of indexes better than
   // single indexes, so try to portion items that need to be toggled into ranges.
   bool formingToggleIndexRange = false;
   QModelIndex toggleIndexRangeBegin = QModelIndex();

   do {
       QRect currIndexRect = visualRect(currIndex);

        // Update some optimization info as we go.
       const int cr = currIndexRect.right();
       const int cl = currIndexRect.left();
       const int sl = selRect.left();
       const int sr = selRect.right();
       // "The right edge of the name is outside of the rect but nearer than m_outsideNearestLeft", etc
       if ((cr < sl && cr > m_band.outsideNearestLeftEdge)) {
           m_band.outsideNearestLeftEdge = cr;
       }
       if ((cl > sr && cl < m_band.outsideNearestRightEdge)) {
           m_band.outsideNearestRightEdge = cl;
       }
       if ((cl >= sl && cl <= sr && cl > m_band.insideNearestRightEdge)) {
           m_band.insideNearestRightEdge = cl;
       }
       if ((cr >= sl && cr <= sr && cr < m_band.insideNearestLeftEdge)) {
           m_band.insideNearestLeftEdge = cr;
       }

       bool currentlySelected = selectionModel()->isSelected(currIndex);
       bool originallySelected = m_band.originalSelection.contains(currIndex);
       bool intersectsSelectedRect = currIndexRect.intersects(selRect);
       bool shouldBeSelected = (intersectsSelectedRect && !originallySelected) || (!intersectsSelectedRect && originallySelected);
       bool needToToggleItem = (currentlySelected && !shouldBeSelected) || (!currentlySelected && shouldBeSelected);
       if (needToToggleItem && !formingToggleIndexRange) {
            toggleIndexRangeBegin = currIndex;
            formingToggleIndexRange = true;
       }

       // NOTE: indexBelow actually walks up and down expanded trees for us.
       QModelIndex nextIndex = indexBelow(currIndex);
       allItemsInBoundDone = !nextIndex.isValid() || currIndexRect.top() > verticalCutoff;

       const bool commitToggleIndexRange = formingToggleIndexRange &&
                                           (!needToToggleItem ||
                                            allItemsInBoundDone ||
                                            currIndex.parent() != toggleIndexRangeBegin.parent());
       if (commitToggleIndexRange) {
           formingToggleIndexRange = false;
            // If this is the last item in the bounds and it is also the beginning of a range,
            // don't toggle lastIndex - it will already have been dealt with.
           if (!allItemsInBoundDone || toggleIndexRangeBegin != currIndex) {
               itemsToToggle.select(toggleIndexRangeBegin, lastIndex);
           }
            // Need to start a new range immediately with currIndex?
           if (needToToggleItem) {
               toggleIndexRangeBegin = currIndex;
               formingToggleIndexRange = true;
           }
           if (allItemsInBoundDone && needToToggleItem) {
                // Toggle the very last item in the bounds.
               itemsToToggle.select(currIndex, currIndex);
           }
       }

       // next item
       lastIndex = currIndex;
       currIndex = nextIndex;
    } while (!allItemsInBoundDone);

    selectionModel()->select(itemsToToggle, QItemSelectionModel::Toggle);

    m_band.lastSelectionOrigin = m_band.origin;
    m_band.lastSelectionDestination = m_band.destination;
    m_band.ignoreOldInfo = false;
}

void DolphinDetailsView::setFoldersExpandable(bool expandable)
{
    if (!expandable) {
        // collapse all expanded folders, as QTreeView::setItemsExpandable(false)
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
}

void DolphinDetailsView::slotExpanded(const QModelIndex& index)
{
    KFileItem item = m_controller->itemForIndex(index);
    if (!item.isNull()) {
        m_expandedUrls.insert(item.url());
    }
}

void DolphinDetailsView::slotCollapsed(const QModelIndex& index)
{
    KFileItem item = m_controller->itemForIndex(index);
    if (!item.isNull()) {
        m_expandedUrls.remove(item.url());
    }
}

void DolphinDetailsView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    removeExpandedIndexes(parent, start, end);
    QTreeView::rowsAboutToBeRemoved(parent, start, end);
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

    doItemsLayout();
}

KFileItemDelegate::Information DolphinDetailsView::infoForColumn(int columnIndex) const
{
    KFileItemDelegate::Information info = KFileItemDelegate::NoInformation;

    switch (columnIndex) {
    case DolphinModel::Size:         info = KFileItemDelegate::Size; break;
    case DolphinModel::ModifiedTime: info = KFileItemDelegate::ModificationTime; break;
    case DolphinModel::Permissions:  info = KFileItemDelegate::Permissions; break;
    case DolphinModel::Owner:        info = KFileItemDelegate::Owner; break;
    case DolphinModel::Group:        info = KFileItemDelegate::OwnerAndGroup; break;
    case DolphinModel::Type:         info = KFileItemDelegate::FriendlyMimeType; break;
    default: break;
    }

    return info;
}

void DolphinDetailsView::resizeColumns()
{
    // Using the resize mode QHeaderView::ResizeToContents is too slow (it takes
    // around 3 seconds for each (!) resize operation when having > 10000 items).
    // This gets a problem especially when opening large directories, where several
    // resize operations are received for showing the currently available items during
    // loading (the application hangs around 20 seconds when loading > 10000 items).

    QHeaderView* headerView = header();
    QFontMetrics fontMetrics(viewport()->font());

    int columnWidth[DolphinModel::Version + 1];
    columnWidth[DolphinModel::Size] = fontMetrics.width("00000 Items");
    columnWidth[DolphinModel::ModifiedTime] = fontMetrics.width("0000-00-00 00:00");
    columnWidth[DolphinModel::Permissions] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[DolphinModel::Owner] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[DolphinModel::Group] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[DolphinModel::Type] = fontMetrics.width("XXXX Xxxxxxx");
    columnWidth[DolphinModel::Version] = fontMetrics.width("xxxxxxxx");

    int requiredWidth = 0;
    for (int i = KDirModel::Size; i <= KDirModel::Type; ++i) {
        if (!isColumnHidden(i)) {
            columnWidth[i] += 20; // provide a default gap
            requiredWidth += columnWidth[i];
            headerView->resizeSection(i, columnWidth[i]);
        }
    }

    // resize the name column in a way that the whole available width is used
    columnWidth[KDirModel::Name] = viewport()->width() - requiredWidth;

    const int minNameWidth = 300;
    if (columnWidth[KDirModel::Name] < minNameWidth) {
        columnWidth[KDirModel::Name] = minNameWidth;

        // It might be possible that the name column width can be
        // decreased without clipping any text. For performance
        // reasons the exact necessary width for full visible names is
        // only checked for up to 200 items:
        const int rowCount = model()->rowCount();
        if (rowCount > 0 && rowCount < 200) {
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

bool DolphinDetailsView::isAboveExpandingToggle(const QPoint& pos) const
{
    // QTreeView offers no public API to get the information whether an index has an
    // expanding toggle and what boundaries the toggle has. The following approach
    // also assumes a toggle for file items.
    if (itemsExpandable()) {
        const QModelIndex index = QTreeView::indexAt(pos);
        if (index.isValid() && (index.column() == KDirModel::Name)) {
            QRect rect = visualRect(index);
            const int toggleSize = rect.height();
            if (isRightToLeft()) {
                rect.moveRight(rect.right());
            } else {
                rect.moveLeft(rect.x() - toggleSize);
            }
            rect.setWidth(toggleSize);

            QStyleOption opt;
            opt.initFrom(this);
            opt.rect = rect;
            rect = style()->subElementRect(QStyle::SE_TreeViewDisclosureItem, &opt, this);

            return rect.contains(pos);
        }
    }
    return false;
}

DolphinDetailsView::ElasticBand::ElasticBand() :
    show(false),
    origin(),
    destination(),
    lastSelectionOrigin(),
    lastSelectionDestination(),
    ignoreOldInfo(true),
    outsideNearestLeftEdge(0),
    outsideNearestRightEdge(0),
    insideNearestLeftEdge(0),
    insideNearestRightEdge(0)
{
}

#include "dolphindetailsview.moc"
