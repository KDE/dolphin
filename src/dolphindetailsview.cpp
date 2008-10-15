/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
    m_controller(controller),
    m_selectionManager(0),
    m_font(),
    m_decorationSize(),
    m_showElasticBand(false),
    m_elasticBandOrigin(),
    m_elasticBandDestination()
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

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick' and to handle the
    // RETURN-key in keyPressEvent().
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
    } else if (event->type() == QEvent::UpdateRequest) {
        // a wheel movement will scroll 4 items
        if (model()->rowCount() > 0) {
            verticalScrollBar()->setSingleStep((sizeHintForRow(0) / 3) * 4);
        }
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

        // restore the current index, other columns are handled as viewport area
        selectionModel()->setCurrentIndex(current, QItemSelectionModel::Current);
    }

    if ((event->button() == Qt::LeftButton) && !m_expandingTogglePressed) {
        m_showElasticBand = true;
        const QPoint pos = contentsPos();
        const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
        m_elasticBandOrigin = event->pos() + pos + scrollPos;
        m_elasticBandDestination = m_elasticBandOrigin;
    }
}

void DolphinDetailsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_showElasticBand) {
        const QPoint mousePos = event->pos();
        const QModelIndex index = indexAt(mousePos);
        if (!index.isValid()) {
            // the destination of the selection rectangle is above the viewport. In this
            // case QTreeView does no selection at all, which is not the wanted behavior
            // in Dolphin -> select all items within the elastic band rectangle
            clearSelection();
            setState(DragSelectingState);

            const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
            QRect selRect = elasticBandRect();
            const QRect nameColumnsRect(0, 0, nameColumnWidth, viewport()->height());
            selRect = nameColumnsRect.intersected(selRect);

            setSelection(selRect, QItemSelectionModel::Select);

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
    if (m_showElasticBand) {
        setState(NoState);
        updateElasticBand();
        m_showElasticBand = false;
    }
}

void DolphinDetailsView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::startDrag(this, supportedActions);
    m_showElasticBand = false;
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    if (m_showElasticBand) {
        updateElasticBand();
        m_showElasticBand = false;
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

    if (event->mimeData()->hasUrls()) {
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
    if (m_showElasticBand) {
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
    if (m_autoResize) {
        resizeColumns();
    }
    QTreeView::resizeEvent(event);
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

    QTreeView::wheelEvent(event);
}

void DolphinDetailsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTreeView::currentChanged(current, previous);

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
    if (!m_showElasticBand) {
        m_useDefaultIndexAt = true;
        QTreeView::setSelection(rect, command);
        m_useDefaultIndexAt = false;
    } else {
        // Select the items contained within the elastic band.

        // TODO - would this still work if the columns could be re-ordered
        // (which I want to implement eventually :))?

        // Very naive implementation - clears all selected, then walks a tree,
        // selecting all items whose nameColumnRect(...) intersects rect.

        // Choose a sensible startIndex - a parentless index in the 
        // name column, as close to the top of the rect as we 
        // can find.
        QRect normalisedRect = rect.normalized();
        QModelIndex startIndex = QTreeView::indexAt(normalisedRect.topLeft());
        if (startIndex.isValid()) {
            while (startIndex.parent().isValid()) {
                startIndex = startIndex.parent();
            }
        } else {
            // just pick the topmost row for safety
            model()->index(0, KDirModel::Name);
        }
        startIndex = model()->index(startIndex.row(), KDirModel::Name);
        clearSelection();
        setSelectionRecursive(startIndex, normalisedRect, command);
    }
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
    if (m_showElasticBand) {
        QRect dirtyRegion(elasticBandRect());        
        const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());
        m_elasticBandDestination = viewport()->mapFromGlobal(QCursor::pos()) + scrollPos;
        dirtyRegion = dirtyRegion.united(elasticBandRect());
        setDirtyRegion(dirtyRegion);
    }
}

QRect DolphinDetailsView::elasticBandRect() const
{
    const QPoint pos(contentsPos());
    const QPoint scrollPos(horizontalScrollBar()->value(), verticalScrollBar()->value());

    const QPoint topLeft = m_elasticBandOrigin - pos - scrollPos;
    const QPoint bottomRight = m_elasticBandDestination - pos - scrollPos;
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
    if (QApplication::mouseButtons() & Qt::LeftButton) {
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
        const int width = DolphinFileItemDelegate::nameColumnWidth(item.name(), viewOptions());
        rect.setWidth(width);
    }

    return rect;
}

void DolphinDetailsView::setSelectionRecursive(const QModelIndex& startIndex,
                                               const QRect& rect,
                                               QItemSelectionModel::SelectionFlags command)
{
    if (!startIndex.isValid()) {
        return;
    }
    
    // rect is assumed to be in viewport coordinates and normalized.
    // Move down through the siblings of startIndex, exploring the children
    // of any expanded nodes.
    Q_ASSERT(rect.width() >= 0 && rect.height() >= 0);
    QModelIndex currIndex = startIndex;
    do {
        const QModelIndex belowIndex = indexBelow(currIndex);
        if (isExpanded(currIndex)) {
            // If belowIndex exists and is above the top of rect, then we need not explore
            // the children of currIndex as they will always be above "below".  Otherwise,
            // explore the children.
            if (!belowIndex.isValid() || visualRect(belowIndex).bottom() >= rect.top()) {
                setSelectionRecursive(currIndex.child(0, currIndex.column()), rect, command);
            }
        }

        QRect itemContentRect = nameColumnRect(currIndex);
        if (itemContentRect.top() > rect.bottom()) {
            // All remaining items will be below itemContentRect, so we may cull.
            return;
        }

        if (itemContentRect.intersects(rect)) {
            selectionModel()->select(currIndex, QItemSelectionModel::Select);   
        }

        currIndex = belowIndex;
    } while (currIndex.isValid());
}

#include "dolphindetailsview.moc"
