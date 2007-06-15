/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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

#include "dolphincolumnview.h"

#include "dolphincontroller.h"
#include "dolphinsettings.h"

#include "dolphin_columnmodesettings.h"

#include <QtGui/QAbstractProxyModel>
#include <QtCore/QPoint>

/**
 * Represents one column inside the DolphinColumnView and has been
 * extended to respect view options and hovering information.
 */
class ColumnWidget : public QListView
{
public:
    ColumnWidget(QWidget* parent, DolphinColumnView* columnView);
    virtual ~ColumnWidget();

    void setDecorationSize(const QSize& size);

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);

private:
    DolphinColumnView* m_columnView;
    QStyleOptionViewItem m_viewOptions;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
};

ColumnWidget::ColumnWidget(QWidget* parent, DolphinColumnView* columnView) :
    QListView(parent),
    m_columnView(columnView),
    m_dragging(false),
    m_dropRect()
{
    setAcceptDrops(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

    // apply the column mode settings to the widget
    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = QListView::viewOptions();

    QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;
}

ColumnWidget::~ColumnWidget()
{
}

void ColumnWidget::setDecorationSize(const QSize& size)
{
    m_viewOptions.decorationSize = size;
    doItemsLayout();
}

QStyleOptionViewItem ColumnWidget::viewOptions() const
{
    return m_viewOptions;
}

void ColumnWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    m_dragging = true;
}

void ColumnWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QListView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QListView::dragMoveEvent(event);

   // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();
        m_columnView->m_controller->indicateDroppedUrls(urls,
                                                          indexAt(event->pos()),
                                                          event->source());
    }
    QListView::dropEvent(event);
    m_dragging = false;
}

void ColumnWidget::paintEvent(QPaintEvent* event)
{
    QListView::paintEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    if (m_dragging) {
        const QBrush& brush = m_viewOptions.palette.brush(QPalette::Normal, QPalette::Highlight);
        DolphinController::drawHoverIndication(viewport(), m_dropRect, brush);
    }
}

// ---

DolphinColumnView::DolphinColumnView(QWidget* parent, DolphinController* controller) :
    QColumnView(parent),
    m_controller(controller)
{
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(activated(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));
    connect(this, SIGNAL(entered(const QModelIndex&)),
            controller, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));

    updateDecorationSize();
}

DolphinColumnView::~DolphinColumnView()
{
}

QAbstractItemView* DolphinColumnView::createColumn(const QModelIndex& index)
{
    ColumnWidget* view = new ColumnWidget(viewport(), this);

    // The following code has been copied 1:1 from QColumnView::createColumn().
    // Copyright (C) 1992-2007 Trolltech ASA.
    // It would be nice if QColumnView would offer a protected method for this
    // (already send input to Benjamin, hopefully possible in Qt4.4)

    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setMinimumWidth(100);
    view->setAttribute(Qt::WA_MacShowFocusRect, false);

    // copy the 'view' behavior
    view->setDragDropMode(dragDropMode());
    view->setDragDropOverwriteMode(dragDropOverwriteMode());
    view->setDropIndicatorShown(showDropIndicator());
    view->setAlternatingRowColors(alternatingRowColors());
    view->setAutoScroll(hasAutoScroll());
    view->setEditTriggers(editTriggers());
    view->setHorizontalScrollMode(horizontalScrollMode());
    view->setIconSize(iconSize());
    view->setSelectionBehavior(selectionBehavior());
    view->setSelectionMode(selectionMode());
    view->setTabKeyNavigation(tabKeyNavigation());
    view->setTextElideMode(textElideMode());
    view->setVerticalScrollMode(verticalScrollMode());

    view->setModel(model());

    // set the delegate to be the columnview delegate
    QAbstractItemDelegate *delegate = view->itemDelegate();
    view->setItemDelegate(itemDelegate());
    delete delegate;

    view->setRootIndex(index);

    if (model()->canFetchMore(index)) {
        model()->fetchMore(index);
    }

    return view;
}

void DolphinColumnView::contextMenuEvent(QContextMenuEvent* event)
{
    QColumnView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinColumnView::mousePressEvent(QMouseEvent* event)
{
    m_controller->triggerActivation();
    QColumnView::mousePressEvent(event);
}

void DolphinColumnView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DolphinColumnView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        m_controller->indicateDroppedUrls(urls,
                                          indexAt(event->pos()),
                                          event->source());
        event->acceptProposedAction();
    }
    QColumnView::dropEvent(event);
}

void DolphinColumnView::zoomIn()
{
    if (isZoomInPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeSmall:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::zoomOut()
{
    if (isZoomOutPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeLarge:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

bool DolphinColumnView::isZoomInPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() < K3Icon::SizeLarge;
}

bool DolphinColumnView::isZoomOutPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() > K3Icon::SizeSmall;
}

void DolphinColumnView::updateDecorationSize()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int iconSize = settings->iconSize();

    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            ColumnWidget* widget = static_cast<ColumnWidget*>(object);
            widget->setDecorationSize(QSize(iconSize, iconSize));
        }
    }

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    doItemsLayout();
}

#include "dolphincolumnview.moc"
