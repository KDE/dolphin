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

#include "dolphin_iconsmodesettings.h"

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>

#include <QAbstractProxyModel>
#include <QPoint>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    QListView(parent),
    m_controller(controller)
{
    Q_ASSERT(controller != 0);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);

    viewport()->setAttribute(Qt::WA_Hover);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));
    connect(this, SIGNAL(activated(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));
    connect(controller, SIGNAL(showPreviewChanged(bool)),
            this, SLOT(updateGridSize(bool)));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));

    // apply the icons mode settings to the widget
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = QListView::viewOptions();
    m_viewOptions.font = QFont(settings->fontFamily(), settings->fontSize());

    updateGridSize(controller->showPreview());

    if (settings->arrangement() == QListView::TopToBottom) {
        setFlow(QListView::LeftToRight);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Top;
    }
    else {
        setFlow(QListView::TopToBottom);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Left;
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
    QListView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinIconsView::mouseReleaseEvent(QMouseEvent* event)
{
    QListView::mouseReleaseEvent(event);
    m_controller->triggerActivation();
}

void DolphinIconsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
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
    QListView::dropEvent(event);
}

void DolphinIconsView::updateGridSize(bool showPreview)
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    int gridWidth = settings->gridWidth();
    int gridHeight = settings->gridHeight();
    int size = settings->iconSize();

    if (showPreview) {
        const int previewSize = settings->previewSize();
        const int diff = previewSize - size;
        Q_ASSERT(diff >= 0);
        gridWidth += diff;
        gridHeight += diff;

        size = previewSize;
     }


    m_viewOptions.decorationSize = QSize(size, size);
    setGridSize(QSize(gridWidth, gridHeight));

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());
}

void DolphinIconsView::zoomIn()
{
    if (isZoomInPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const bool showPreview = m_controller->showPreview();
        if (showPreview) {
            const int previewSize = increasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
        }
        else {
            const int iconSize = increasedIconSize(settings->iconSize());
            settings->setIconSize(iconSize);
            if (settings->previewSize() < iconSize) {
                // assure that the preview size is always >= the icon size
                settings->setPreviewSize(iconSize);
            }
        }

        updateGridSize(showPreview);
    }
}

void DolphinIconsView::zoomOut()
{
    if (isZoomOutPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const bool showPreview = m_controller->showPreview();
        if (showPreview) {
            const int previewSize = decreasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
            if (settings->iconSize() > previewSize) {
                // assure that the icon size is always <= the preview size
                settings->setIconSize(previewSize);
            }
        }
        else {
            const int iconSize = decreasedIconSize(settings->iconSize());
            settings->setIconSize(iconSize);
        }

        updateGridSize(showPreview);
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

#include "dolphiniconsview.moc"
