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

#include "dolphiniconsview.h"
#include "dolphinview.h"

#include <kdirmodel.h>
#include <kfileitem.h>

DolphinIconsView::DolphinIconsView(DolphinView* parent) :
    QListView(parent),
    m_parentView( parent )
{
    setResizeMode(QListView::Adjust);
}

DolphinIconsView::~DolphinIconsView()
{
}

QStyleOptionViewItem DolphinIconsView::viewOptions() const
{
    return QListView::viewOptions();

    // TODO: the view options should been read from the settings;
    // the following code is just for testing...
    //QStyleOptionViewItem options = QListView::viewOptions();
    //options.decorationAlignment = Qt::AlignRight;
    //options.decorationPosition = QStyleOptionViewItem::Right;
    //options.decorationSize = QSize(100, 100);
    //options.showDecorationSelected = true;
    //options.state = QStyle::State_MouseOver;
    //return options;
}

void DolphinIconsView::contextMenuEvent(QContextMenuEvent* event)
{
    QListView::contextMenuEvent(event);

    KFileItem* item = 0;

    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        // TODO: assuming that model() returns an instance of the class
        // KDirModel is dangerous, especially in combination with a proxy model.
        // As the current test implementation of proxy model does not work, this
        // will be cleaned up later.
        KDirModel* dirModel = static_cast<KDirModel*>(model());
        item = dirModel->itemForIndex(index);
    }

    m_parentView->openContextMenu(item, event->globalPos());
}

void DolphinIconsView::mouseReleaseEvent(QMouseEvent* event)
{
    QListView::mouseReleaseEvent(event);
    m_parentView->declareViewActive();
}

#include "dolphiniconsview.moc"
