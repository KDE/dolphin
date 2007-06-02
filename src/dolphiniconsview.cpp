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

#include "dolphiniconsview.h"

#include "dolphincontroller.h"
#include "dolphinsettings.h"
#include "dolphinitemcategorizer.h"

#include "dolphin_iconsmodesettings.h"

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QPainter>
#include <QPoint>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    KListView(parent),
    m_controller(controller),
    m_dragging(false)
{
    Q_ASSERT(controller != 0);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);

    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

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
    connect(controller, SIGNAL(showPreviewChanged(bool)),
            this, SLOT(slotShowPreviewChanged(bool)));
    connect(controller, SIGNAL(showAdditionalInfoChanged(bool)),
            this, SLOT(slotShowAdditionalInfoChanged(bool)));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));

    // apply the icons mode settings to the widget
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = KListView::viewOptions();
    m_viewOptions.showDecorationSelected = true;

    QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;

    setWordWrap(settings->numberOfTextlines() > 1);
    updateGridSize(controller->showPreview(), controller->showAdditionalInfo());

    if (settings->arrangement() == QListView::TopToBottom) {
        setFlow(QListView::LeftToRight);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Top;
    } else {
        setFlow(QListView::TopToBottom);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Left;
        m_viewOptions.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }
}

DolphinIconsView::~DolphinIconsView()
{
}

QStyleOptionViewItem DolphinIconsView::viewOptions() const
{
    return m_viewOptions;
}

void DolphinIconsView::contextMenuEvent(QContextMenuEvent* event)
{
    KListView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinIconsView::mousePressEvent(QMouseEvent* event)
{
    if (!indexAt(event->pos()).isValid()) {
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if (!(modifier & Qt::ShiftModifier) && !(modifier & Qt::ControlModifier)) {
            clearSelection();
        }
    }

    KListView::mousePressEvent(event);
}

void DolphinIconsView::mouseReleaseEvent(QMouseEvent* event)
{
    KListView::mouseReleaseEvent(event);
    m_controller->triggerActivation();
}

void DolphinIconsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    m_dragging = true;
}

void DolphinIconsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    KListView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dragMoveEvent(QDragMoveEvent* event)
{
    KListView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        m_controller->indicateDroppedUrls(urls,
                                          indexAt(event->pos()),
                                          event->source());
        event->acceptProposedAction();
    }
    KListView::dropEvent(event);
    m_dragging = false;
}

void DolphinIconsView::paintEvent(QPaintEvent* event)
{
    KListView::paintEvent(event);

    if (m_dragging) {
        // TODO: remove this code when the issue #160611 is solved in Qt 4.4
        QPainter painter(viewport());
        painter.save();
        QBrush brush(m_viewOptions.palette.brush(QPalette::Normal, QPalette::Highlight));
        QColor color = brush.color();
        color.setAlpha(64);
        brush.setColor(color);
        painter.fillRect(m_dropRect, brush);
        painter.restore();
    }
}

void DolphinIconsView::slotShowPreviewChanged(bool showPreview)
{
    updateGridSize(showPreview, m_controller->showAdditionalInfo());
}

void DolphinIconsView::slotShowAdditionalInfoChanged(bool showAdditionalInfo)
{
    updateGridSize(m_controller->showPreview(), showAdditionalInfo);
}

void DolphinIconsView::zoomIn()
{
    if (isZoomInPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const int oldIconSize = settings->iconSize();
        int newIconSize = oldIconSize;

        const bool showPreview = m_controller->showPreview();
        if (showPreview) {
            const int previewSize = increasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
        } else {
            newIconSize = increasedIconSize(oldIconSize);
            settings->setIconSize(newIconSize);
            if (settings->previewSize() < newIconSize) {
                // assure that the preview size is always >= the icon size
                settings->setPreviewSize(newIconSize);
            }
        }

        // increase also the grid size
        const int diff = newIconSize - oldIconSize;
        settings->setItemWidth(settings->itemWidth() + diff);
        settings->setItemHeight(settings->itemHeight() + diff);

        updateGridSize(showPreview, m_controller->showAdditionalInfo());
    }
}

void DolphinIconsView::zoomOut()
{
    if (isZoomOutPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const int oldIconSize = settings->iconSize();
        int newIconSize = oldIconSize;

        const bool showPreview = m_controller->showPreview();
        if (showPreview) {
            const int previewSize = decreasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
            if (settings->iconSize() > previewSize) {
                // assure that the icon size is always <= the preview size
                newIconSize = previewSize;
                settings->setIconSize(newIconSize);
            }
        } else {
            newIconSize = decreasedIconSize(settings->iconSize());
            settings->setIconSize(newIconSize);
        }

        // decrease also the grid size
        const int diff = oldIconSize - newIconSize;
        settings->setItemWidth(settings->itemWidth() - diff);
        settings->setItemHeight(settings->itemHeight() - diff);

        updateGridSize(showPreview, m_controller->showAdditionalInfo());
    }
}

bool DolphinIconsView::isZoomInPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const int size = m_controller->showPreview() ? settings->previewSize() : settings->iconSize();
    return size < K3Icon::SizeEnormous;
}

bool DolphinIconsView::isZoomOutPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const int size = m_controller->showPreview() ? settings->previewSize() : settings->iconSize();
    return size > K3Icon::SizeSmall;
}

int DolphinIconsView::increasedIconSize(int size) const
{
    // TODO: get rid of K3Icon sizes
    int incSize = 0;
    switch (size) {
    case K3Icon::SizeSmall:       incSize = K3Icon::SizeSmallMedium; break;
    case K3Icon::SizeSmallMedium: incSize = K3Icon::SizeMedium; break;
    case K3Icon::SizeMedium:      incSize = K3Icon::SizeLarge; break;
    case K3Icon::SizeLarge:       incSize = K3Icon::SizeHuge; break;
    case K3Icon::SizeHuge:        incSize = K3Icon::SizeEnormous; break;
    default: Q_ASSERT(false); break;
    }
    return incSize;
}

int DolphinIconsView::decreasedIconSize(int size) const
{
    // TODO: get rid of K3Icon sizes
    int decSize = 0;
    switch (size) {
    case K3Icon::SizeSmallMedium: decSize = K3Icon::SizeSmall; break;
    case K3Icon::SizeMedium: decSize = K3Icon::SizeSmallMedium; break;
    case K3Icon::SizeLarge: decSize = K3Icon::SizeMedium; break;
    case K3Icon::SizeHuge: decSize = K3Icon::SizeLarge; break;
    case K3Icon::SizeEnormous: decSize = K3Icon::SizeHuge; break;
    default: Q_ASSERT(false); break;
    }
    return decSize;
}

void DolphinIconsView::updateGridSize(bool showPreview, bool showAdditionalInfo)
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    int itemWidth = settings->itemWidth();
    int itemHeight = settings->itemHeight();
    int size = settings->iconSize();

    if (showPreview) {
        const int previewSize = settings->previewSize();
        const int diff = previewSize - size;
        Q_ASSERT(diff >= 0);
        itemWidth  += diff;
        itemHeight += diff;

        size = previewSize;
    }

    if (showAdditionalInfo) {
        itemHeight += m_viewOptions.font.pointSize() * 2;
    }

    if (settings->arrangement() == QListView::TopToBottom) {
        // The decoration width indirectly defines the maximum
        // width for the text wrapping. To use the maximum item width
        // for text wrapping, it is used as decoration width.
        m_viewOptions.decorationSize = QSize(itemWidth, size);
    } else {
        m_viewOptions.decorationSize = QSize(size, size);
    }

    const int spacing = settings->gridSpacing();
    setGridSize(QSize(itemWidth + spacing, itemHeight + spacing));

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());
}

#include "dolphiniconsview.moc"
