/***************************************************************************
 *   Copyright (C) 2009 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#include "dolphindetailsviewexpander.h"

#include "dolphindetailsview.h"
#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include <KDirLister>
#include <KDirModel>

DolphinDetailsViewExpander::DolphinDetailsViewExpander(DolphinDetailsView* parent,
                                                       const QSet<KUrl>& urlsToExpand) :
    QObject(parent),
    m_detailsView(parent),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0)
{
    Q_ASSERT(parent);

    m_proxyModel = qobject_cast<const DolphinSortFilterProxyModel*>(parent->model());
    Q_ASSERT(m_proxyModel);

    m_dolphinModel = qobject_cast<const DolphinModel*>(m_proxyModel->sourceModel());
    Q_ASSERT(m_dolphinModel);

    m_dirLister = m_dolphinModel->dirLister();
    Q_ASSERT(m_dirLister);

    // The URLs must be sorted. E.g. /home/user/ cannot be expanded before /home/
    // because it is not known to the dir model before.
    m_urlsToExpand = urlsToExpand.toList();
    qSort(m_urlsToExpand);

    // The dir lister must have completed the folder listing before a subfolder can be expanded.
    connect(m_dirLister, SIGNAL(completed()), this, SLOT(slotDirListerCompleted()));
}

DolphinDetailsViewExpander::~DolphinDetailsViewExpander()
{
}

void DolphinDetailsViewExpander::stop()
{
    disconnect(m_dirLister, SIGNAL(completed()), this, SLOT(slotDirListerCompleted()));
    deleteLater();
}

void DolphinDetailsViewExpander::slotDirListerCompleted()
{
    QModelIndex dirIndex;

    while(!m_urlsToExpand.isEmpty() && !dirIndex.isValid()) {
        const KUrl url = m_urlsToExpand.takeFirst();
        dirIndex = m_dolphinModel->indexForUrl(url);
    }

    if(dirIndex.isValid()) {
        // A valid model index was found. Note that only one item is expanded in each call of this slot
        // because expanding any item will trigger KDirLister::openUrl(...) via KDirModel::fetchMore(...),
        // and we can only continue when the dir lister is done.
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        m_detailsView->expand(proxyIndex);
    }
    else {
        emit completed();
        stop();
    }
}
