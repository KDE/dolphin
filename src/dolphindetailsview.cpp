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

#include "dolphincontroller.h"
#include "dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "viewproperties.h"

#include "dolphin_detailsmodesettings.h"

#include <assert.h>
#include <kdirmodel.h>
#include <QHeaderView>

DolphinDetailsView::DolphinDetailsView(QWidget* parent, DolphinController* controller) :
    QTreeView(parent),
    m_controller(controller)
{
    assert(controller != 0);

    setAcceptDrops(true);
    setRootIsDecorated(false);
    setSortingEnabled(true);
    setUniformRowHeights(true);

    const ViewProperties props(controller->url());
    setSortIndicatorSection(props.sorting());
    setSortIndicatorOrder(props.sortOrder());

    connect(header(), SIGNAL(sectionClicked(int)),
            this, SLOT(synchronizeSortingState(int)));

    connect(parent, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(setSortIndicatorSection(DolphinView::Sorting)));
    connect(parent, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(setSortIndicatorOrder(Qt::SortOrder)));

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            controller, SLOT(triggerItem(const QModelIndex&)));

   // apply the details mode settings to the widget
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    assert(settings != 0);

    m_viewOptions = QTreeView::viewOptions();
    m_viewOptions.font = QFont(settings->fontFamily(), settings->fontSize());
    const int iconSize = settings->iconSize();
    m_viewOptions.decorationSize = QSize(iconSize, iconSize);
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

        // hide columns if this is indicated by the settings
        const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        assert(settings != 0);
        if (!settings->showDate()) {
            hideColumn(KDirModel::ModifiedTime);
        }

        if (!settings->showPermissions()) {
            hideColumn(KDirModel::Permissions);
        }

        if (!settings->showOwner()) {
            hideColumn(KDirModel::Owner);
        }

        if (!settings->showGroup()) {
            hideColumn(KDirModel::Group);
        }
    }

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

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
    m_controller->triggerActivation();
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (urls.isEmpty() || (event->source() == this)) {
        QTreeView::dropEvent(event);
    }
    else {
        event->acceptProposedAction();
        m_controller->indicateDroppedUrls(urls, event->pos());
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

#include "dolphindetailsview.moc"
