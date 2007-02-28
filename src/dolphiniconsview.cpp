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

#include <assert.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>

#include <QAbstractProxyModel>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    QListView(parent),
    m_controller(controller)
{
    Q_ASSERT(controller != 0);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));
    connect(controller, SIGNAL(showPreviewChanged(bool)),
            this, SLOT(updateGridSize(bool)));

    // apply the icons mode settings to the widget
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    setSpacing(settings->gridSpacing());

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

    KFileItemDelegate* delegate = new KFileItemDelegate(parent);
    const KFileItemDelegate::AdditionalInformation info =
        static_cast<KFileItemDelegate::AdditionalInformation>(settings->additionalInfo());
    delegate->setAdditionalInformation(info);
    setItemDelegate(delegate);
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
    if (urls.isEmpty() || (event->source() == this)) {
        QListView::dropEvent(event);
    }
    else {
        event->acceptProposedAction();
        m_controller->indicateDroppedUrls(urls, event->pos());
    }
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
}

#include "dolphiniconsview.moc"
