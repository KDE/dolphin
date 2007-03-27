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

//#include "dolphin_iconsmodesettings.h"

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>

#include <QAbstractProxyModel>
#include <QPoint>

DolphinColumnView::DolphinColumnView(QWidget* parent, DolphinController* controller) :
    QColumnView(parent),
    m_controller(controller)
{
    Q_ASSERT(controller != 0);

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
    //const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    //Q_ASSERT(settings != 0);

    m_viewOptions = QColumnView::viewOptions();

    /*QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;

    updateGridSize(controller->showPreview());

    if (settings->arrangement() == QColumnView::TopToBottom) {
        setFlow(QColumnView::LeftToRight);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Top;
    }
    else {
        setFlow(QColumnView::TopToBottom);
        m_viewOptions.decorationPosition = QStyleOptionViewItem::Left;
    }*/
}

DolphinColumnView::~DolphinColumnView()
{
}

QStyleOptionViewItem DolphinColumnView::viewOptions() const
{
    return m_viewOptions;
}

void DolphinColumnView::contextMenuEvent(QContextMenuEvent* event)
{
    QColumnView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinColumnView::mouseReleaseEvent(QMouseEvent* event)
{
    QColumnView::mouseReleaseEvent(event);
    m_controller->triggerActivation();
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
}

void DolphinColumnView::zoomOut()
{
}

bool DolphinColumnView::isZoomInPossible() const
{
    return false;
}

bool DolphinColumnView::isZoomOutPossible() const
{
    return false;
}

#include "dolphincolumnview.moc"
