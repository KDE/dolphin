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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphiniconsview.h"
#include "dolphinview.h"

#include <kdirmodel.h>
#include <kfileitem.h>

DolphinIconsView::DolphinIconsView(DolphinView* parent) :
    QListView(parent),
    m_parentView( parent )
{
    setResizeMode( QListView::Adjust );
}

DolphinIconsView::~DolphinIconsView()
{
}

void DolphinIconsView::mousePressEvent(QMouseEvent* event)
{
    QListView::mousePressEvent(event);

    if (event->button() != Qt::RightButton) {
        return;
    }

    KFileItem* item = 0;

    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
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
