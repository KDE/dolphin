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
#include "dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "viewproperties.h"

#include "dolphin_detailsmodesettings.h"

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
    m_controller(controller),
    m_dragging(false),
    m_showElasticBand(false),
    m_elasticBandOrigin(),
    m_elasticBandDestination()
{
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setRootIsDecorated(false);
    setSortingEnabled(true);
    setUniformRowHeights(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setAlternatingRowColors(true);

    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

    const ViewProperties props(controller->url());
    setSortIndicatorSection(props.sorting());
    setSortIndicatorOrder(props.sortOrder());

    QHeaderView* headerView = header();
    connect(headerView, SIGNAL(sectionClicked(int)),
            this, SLOT(synchronizeSortingState(int)));
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(configureColumns(const QPoint&)));

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
                this, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));
    connect(controller->dolphinView(), SIGNAL(additionalInfoChanged(const KFileItemDelegate::InformationList&)),
            this, SLOT(updateColumnVisibility()));

    // apply the details mode settings to the widget
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = QTreeView::viewOptions();

    QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;
    m_viewOptions.showDecorationSelected = true;

// TODO: Remove this check when 4.3.2 is released and KDE requires it... this
//       check avoids a division by zero happening on versions before 4.3.1.
//       Right now KDE in theory can be shipped with Qt 4.3.0 and above.
//       ereslibre
#if (QT_VERSION >= QT_VERSION_CHECK(4, 3, 2) || defined(QT_KDE_QT_COPY))
    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    setHorizontalScrollMode(QTreeView::ScrollPerPixel);
#endif

    updateDecorationSize();

    setFocus();
}

DolphinDetailsView::~DolphinDetailsView()
{
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        // Assure that by respecting the available width that:
        // - the 'Name' column is stretched as large as possible
        // - the remaining columns are as small as possible
        QHeaderView* headerView = header();
        headerView->setStretchLastSection(false);
        headerView->setResizeMode(QHeaderView::ResizeToContents);
        headerView->setResizeMode(0, QHeaderView::Stretch);
        headerView->setMovable(false);

        updateColumnVisibility();

        hideColumn(DolphinModel::Rating);
        hideColumn(DolphinModel::Tags);
    }
// TODO: Remove this check when 4.3.2 is released and KDE requires it... this
//       check avoids a division by zero happening on versions before 4.3.1.
//       Right now KDE in theory can be shipped with Qt 4.3.0 and above.
//       ereslibre
#if (QT_VERSION >= QT_VERSION_CHECK(4, 3, 2) || defined(QT_KDE_QT_COPY))
    else if (event->type() == QEvent::UpdateRequest) {
        // a wheel movement will scroll 4 items
        if (model()->rowCount() > 0) {
            verticalScrollBar()->setSingleStep((sizeHintForRow(0) / 3) * 4);
        }
    }
#endif

    return QTreeView::event(event);
}

QStyleOptionViewItem DolphinDetailsView::viewOptions() const
{
    return m_viewOptions;
}

void DolphinDetailsView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinDetailsView::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();

    QTreeView::mousePressEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if (!(modifier & Qt::ShiftModifier) && !(modifier & Qt::ControlModifier)) {
            clearSelection();
        }
    }

    if (event->button() == Qt::LeftButton) {
        m_showElasticBand = true;

        const QPoint pos(contentsPos());
        m_elasticBandOrigin = event->pos();
        m_elasticBandOrigin.setX(m_elasticBandOrigin.x() + pos.x());
        m_elasticBandOrigin.setY(m_elasticBandOrigin.y() + pos.y());
        m_elasticBandDestination = event->pos();
    }
}

void DolphinDetailsView::mouseMoveEvent(QMouseEvent* event)
{
    QTreeView::mouseMoveEvent(event);
    if (m_showElasticBand) {
        updateElasticBand();
    }
}

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
    if (m_showElasticBand) {
        updateElasticBand();
        m_showElasticBand = false;
    }
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
    m_dragging = true;
}

void DolphinDetailsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void DolphinDetailsView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    setDirtyRegion(m_dropRect);
    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        m_dragging = false;
    } else {
        m_dragging = true;
        m_dropRect = visualRect(index);
        setDirtyRegion(m_dropRect);
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && (index.column() == DolphinModel::Name)) {
            const KFileItem item = itemForIndex(index);
            m_controller->indicateDroppedUrls(urls,
                                              m_controller->url(),
                                              item,
                                              event->source());
        }
    }
    QTreeView::dropEvent(event);
    m_dragging = false;
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

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    if (m_dragging) {
        const QBrush& brush = m_viewOptions.palette.brush(QPalette::Normal, QPalette::Highlight);
        DolphinController::drawHoverIndication(viewport(), m_dropRect, brush);
    }
}

void DolphinDetailsView::keyPressEvent(QKeyEvent* event)
{
    QTreeView::keyPressEvent(event);

    const QItemSelectionModel* selModel = selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool trigger = currentIndex.isValid()
                         && (event->key() == Qt::Key_Return)
                         && (selModel->selectedIndexes().count() <= 1);
    if (trigger) {
        triggerItem(currentIndex);
    }
}

void DolphinDetailsView::resizeEvent(QResizeEvent* event)
{
    QTreeView::resizeEvent(event);

    // assure that the width of the name-column does not get too small
    const int minWidth = 120;
    QHeaderView* headerView = header();
    bool useFixedWidth = (headerView->sectionSize(KDirModel::Name) <= minWidth)
                         && (headerView->resizeMode(0) != QHeaderView::Fixed);
    if (useFixedWidth) {
        // the current width of the name-column is too small, hence
        // use a fixed size
        headerView->setResizeMode(QHeaderView::Fixed);
        headerView->setResizeMode(0, QHeaderView::Fixed);
        headerView->resizeSection(KDirModel::Name, minWidth);
    } else if (headerView->resizeMode(0) != QHeaderView::Stretch) {
        // check whether there is enough available viewport width
        // to automatically resize the columns
        const int availableWidth = viewport()->width();

        int headerWidth = 0;
        const int count = headerView->count();
        for (int i = 0; i < count; ++i) {
            headerWidth += headerView->sectionSize(i);
        }

        if (headerWidth < availableWidth) {
            headerView->setResizeMode(QHeaderView::ResizeToContents);
            headerView->setResizeMode(0, QHeaderView::Stretch);
        }
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
    const QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
    if (pos.x() < nameColumnWidth) {
        m_controller->emitItemEntered(itemForIndex(index));
    }
    else {
        m_controller->emitViewportEntered();
    }
}

void DolphinDetailsView::updateElasticBand()
{
    Q_ASSERT(m_showElasticBand);
    QRect dirtyRegion(elasticBandRect());
    m_elasticBandDestination = viewport()->mapFromGlobal(QCursor::pos());
    dirtyRegion = dirtyRegion.united(elasticBandRect());
    setDirtyRegion(dirtyRegion);
}

QRect DolphinDetailsView::elasticBandRect() const
{
    const QPoint pos(contentsPos());
    const QPoint topLeft(m_elasticBandOrigin.x() - pos.x(), m_elasticBandOrigin.y() - pos.y());
    return QRect(topLeft, m_elasticBandDestination).normalized();
}

void DolphinDetailsView::zoomIn()
{
    if (isZoomInPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
        case KIconLoader::SizeSmall:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinDetailsView::zoomOut()
{
    if (isZoomOutPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
        case KIconLoader::SizeLarge:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinDetailsView::triggerItem(const QModelIndex& index)
{
    const KFileItem item = itemForIndex(index);
    if (index.isValid() && (index.column() == KDirModel::Name)) {
        m_controller->triggerItem(item);
    } else {
        clearSelection();
        m_controller->emitItemEntered(item);
    }
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
    KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
    if (list.isEmpty() || list.contains(KFileItemDelegate::NoInformation)) {
        list.clear();
        list.append(KFileItemDelegate::Size);
        list.append(KFileItemDelegate::ModificationTime);
        m_controller->indicateAdditionalInfoChange(list);
    }

    setColumnHidden(DolphinModel::Size,         !list.contains(KFileItemDelegate::Size));
    setColumnHidden(DolphinModel::ModifiedTime, !list.contains(KFileItemDelegate::ModificationTime));
    setColumnHidden(DolphinModel::Permissions,  !list.contains(KFileItemDelegate::Permissions));
    setColumnHidden(DolphinModel::Owner,        !list.contains(KFileItemDelegate::Owner));
    setColumnHidden(DolphinModel::Group,        !list.contains(KFileItemDelegate::OwnerAndGroup));
    setColumnHidden(DolphinModel::Type,         !list.contains(KFileItemDelegate::FriendlyMimeType));
}

bool DolphinDetailsView::isZoomInPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() < KIconLoader::SizeLarge;
}

bool DolphinDetailsView::isZoomOutPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() > KIconLoader::SizeSmall;
}

void DolphinDetailsView::updateDecorationSize()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    const int iconSize = settings->iconSize();
    m_viewOptions.decorationSize = QSize(iconSize, iconSize);

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

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

KFileItem DolphinDetailsView::itemForIndex(const QModelIndex& index) const
{
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    return dirModel->itemForIndex(dirIndex);
}

#include "dolphindetailsview.moc"
