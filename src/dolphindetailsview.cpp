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
#include "dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinviewautoscroller.h"
#include "draganddrophelper.h"
#include "selectionmanager.h"
#include "viewproperties.h"
#include "zoomlevelinfo.h"

#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"

#include <kdirmodel.h>
#include <klocale.h>
#include <kmenu.h>

#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QRubberBand>
#include <QPainter>
#include <QScrollBar>

DolphinDetailsView::DolphinDetailsView(QWidget* parent, DolphinController* controller) :
    QTreeView(parent),
    m_autoResize(true),
    m_expandingTogglePressed(false),
    m_keyPressed(false),
    m_useDefaultIndexAt(true),
    m_ignoreScrollTo(false),
    m_controller(controller),
    m_selectionManager(0),
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

    setMouseTracking(true);
    new DolphinViewAutoScroller(this);

    const ViewProperties props(controller->url());
    setSortIndicatorSection(props.sorting());
    setSortIndicatorOrder(props.sortOrder());

    QHeaderView* headerView = header();
    connect(headerView, SIGNAL(sectionClicked(int)),
            this, SLOT(synchronizeSortingState(int)));
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(configureColumns(const QPoint&)));
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

    if (DolphinSettings::instance().generalSettings()->showSelectionToggle()) {
        m_selectionManager = new SelectionManager(this);
        connect(m_selectionManager, SIGNAL(selectionChanged()),
                this, SLOT(requestActivation()));
        connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
                m_selectionManager, SLOT(reset()));
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

    updateDecorationSize(view->showPreview());

    setFocus();
    viewport()->installEventFilter(this);

    connect(KGlobalSettings::self(), SIGNAL(kdisplayFontChanged()),
            this, SLOT(updateFont()));

    m_useDefaultIndexAt = false;
}

DolphinDetailsView::~DolphinDetailsView()
{
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        QHeaderView* headerView = header();
        headerView->setResizeMode(QHeaderView::Interactive);
        headerView->setMovable(false);

        updateColumnVisibility();

        hideColumn(DolphinModel::Rating);
        hideColumn(DolphinModel::Tags);
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
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinDetailsView::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();

    const QModelIndex current = currentIndex();
    QTreeView::mousePressEvent(event);

    m_expandingTogglePressed = false;
    const QModelIndex index = indexAt(event->pos());
    const bool updateState = index.isValid() &&
                             (index.column() == DolphinModel::Name) &&
                             (event->button() == Qt::LeftButton);
    if (updateState) {
        // TODO: See comment in DolphinIconsView::mousePressEvent(). Only update
        // the state if no expanding/collapsing area has been hit:
        const QRect rect = visualRect(index);
        if (event->pos().x() >= rect.x() + indentation()) {
            setState(QAbstractItemView::DraggingState);
        } else {
            m_expandingTogglePressed = true;
            kDebug() << "m_expandingTogglePressed " << m_expandingTogglePressed;
        }
    }

    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        // the mouse press is done somewhere outside the filename column
        if (QApplication::mouseButtons() & Qt::MidButton) {
            m_controller->replaceUrlByClipboard();
        }

        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if (!(modifier & Qt::ShiftModifier) && !(modifier & Qt::ControlModifier)) {
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
            const QPoint pos = contentsPos();
            const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
            m_band.origin = event->pos() + pos + scrollPos;
            m_band.destination = m_band.origin;
            m_band.originalSelection = selectionModel()->selection();
        }
    }
}

void DolphinDetailsView::mouseMoveEvent(QMouseEvent* event)
{
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

    if (m_expandingTogglePressed) {
        // Per default QTreeView starts either a selection or a drag operation when dragging
        // the expanding toggle button (Qt-issue - see TODO comment in DolphinIconsView::mousePressEvent()).
        // Turn off this behavior in Dolphin to stay predictable:
        clearSelection();
        setState(QAbstractItemView::NoState);
    }
}

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
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
    if (m_selectionManager != 0) {
        m_selectionManager->reset();
    }

    // let Ctrl+wheel events propagate to the DolphinView for icon zooming
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }

    const int height = m_decorationSize.height();
    const int step = (height >= KIconLoader::SizeHuge) ? height / 10 : (KIconLoader::SizeHuge - height) / 2;
    verticalScrollBar()->setSingleStep(step);
    QTreeView::wheelEvent(event);
}

void DolphinDetailsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTreeView::currentChanged(current, previous);
    if (current.isValid()) {
        scrollTo(current);
    }

    // Stay consistent with QListView: When changing the current index by key presses,
    // also change the selection.
    if (m_keyPressed) {
        selectionModel()->select(current, QItemSelectionModel::ClearAndSelect);
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
                                    (index.column() == KDirModel::Name) && !nameColumnRect(index).contains(point);
    return isAboveEmptySpace ? QModelIndex() : index;
}

void DolphinDetailsView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
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
    if (m_ignoreScrollTo)
        return;
    QTreeView::scrollTo(index, hint);
}

void DolphinDetailsView::setSortIndicatorSection(DolphinView::Sorting sorting)
{
    QHeaderView* headerView = header();
    headerView->setSortIndicator(sorting, headerView->sortIndicatorOrder());
}

void DolphinDetailsView::setSortIndicatorOrder(Qt::SortOrder sortOrder)
{
    QHeaderView* headerView = header();
    headerView->setSortIndicator(headerView->sortIndicatorSection(), sortOrder);
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
        if (m_band.destination.y() < 0)
            m_band.destination.setY(0);
        if (m_band.destination.x() < 0)
            m_band.destination.setX(0);

        dirtyRegion = dirtyRegion.united(elasticBandRect());
        setDirtyRegion(dirtyRegion);
    }
}

QRect DolphinDetailsView::elasticBandRect() const
{
    const QPoint pos(contentsPos());
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());

    const QPoint topLeft = m_band.origin - pos - scrollPos;
    const QPoint bottomRight = m_band.destination - pos - scrollPos;
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

void DolphinDetailsView::configureColumns(const QPoint& pos)
{
    KMenu popup(this);
    popup.addTitle(i18nc("@title:menu", "Columns"));

    QHeaderView* headerView = header();
    for (int i = DolphinModel::Size; i <= DolphinModel::Type; ++i) {
        const int logicalIndex = headerView->logicalIndex(i);
        const QString text = model()->headerData(i, Qt::Horizontal).toString();
        QAction* action = popup.addAction(text);
        action->setCheckable(true);
        action->setChecked(!headerView->isSectionHidden(logicalIndex));
        action->setData(i);
    }

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
    const KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
    for (int i = DolphinModel::Size; i <= DolphinModel::Type; ++i) {
        const KFileItemDelegate::Information info = infoForColumn(i);
        const bool hide = !list.contains(info);
        if (isColumnHidden(i) != hide) {
            setColumnHidden(i, hide);
        }
    }

    resizeColumns();
}

void DolphinDetailsView::slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    // If the user changes the size of the headers, the autoresize feature should be
    // turned off. As there is no dedicated interface to find out whether the header
    // section has been resized by the user or by a resize event, the following approach is used:
    if ((QApplication::mouseButtons() & Qt::LeftButton) && isVisible()) {
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

void DolphinDetailsView::updateFont()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
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
       QRect currIndexRect = nameColumnRect(currIndex);

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

void DolphinDetailsView::updateDecorationSize(bool showPreview)
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    const int iconSize = showPreview ? settings->previewSize() : settings->iconSize();
    setIconSize(QSize(iconSize, iconSize));
    m_decorationSize = QSize(iconSize, iconSize);

    if (m_selectionManager != 0) {
        m_selectionManager->reset();
    }

    doItemsLayout();
}

QPoint DolphinDetailsView::contentsPos() const
{
    // implementation note: the horizonal position is ignored currently, as no
    // horizontal scrolling is done anyway during a selection
    const QScrollBar* scrollbar = verticalScrollBar();
    Q_ASSERT(scrollbar != 0);

    const int maxHeight = maximumViewportSize().height();
    const int height = scrollbar->maximum() - scrollbar->minimum() + 1;
    const int visibleHeight = model()->rowCount() + 1 - height;
    if (visibleHeight <= 0) {
        return QPoint(0, 0);
    }

    const int y = scrollbar->sliderPosition() * maxHeight / visibleHeight;
    return QPoint(0, y);
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

    int columnWidth[KDirModel::ColumnCount];
    columnWidth[KDirModel::Size] = fontMetrics.width("00000 Items");
    columnWidth[KDirModel::ModifiedTime] = fontMetrics.width("0000-00-00 00:00");
    columnWidth[KDirModel::Permissions] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Owner] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Group] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Type] = fontMetrics.width("XXXX Xxxxxxx");

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
    if (columnWidth[KDirModel::Name] < 120) {
        columnWidth[KDirModel::Name] = 120;
    }
    headerView->resizeSection(KDirModel::Name, columnWidth[KDirModel::Name]);
}

QRect DolphinDetailsView::nameColumnRect(const QModelIndex& index) const
{
    QRect rect = visualRect(index);
    const KFileItem item = m_controller->itemForIndex(index);
    if (!item.isNull()) {
        const int width = DolphinFileItemDelegate::nameColumnWidth(item.text(), viewOptions());
        rect.setWidth(width);
    }

    return rect;
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
