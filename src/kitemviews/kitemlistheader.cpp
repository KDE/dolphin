/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kitemlistheader.h"
#include "kitemlistheaderwidget_p.h"
#include "kitemlistview.h"

KItemListHeader::~KItemListHeader()
{
}

void KItemListHeader::setAutomaticColumnResizing(bool automatic)
{
    if (m_headerWidget->automaticColumnResizing() != automatic) {
        m_headerWidget->setAutomaticColumnResizing(automatic);
        if (automatic) {
            m_view->updateColumnWidthsForHeader();
        }
    }
}

bool KItemListHeader::automaticColumnResizing() const
{
    return m_headerWidget->automaticColumnResizing();
}

void KItemListHeader::setColumnWidth(const QByteArray& role, qreal width)
{
    if (!m_headerWidget->automaticColumnResizing()) {
        m_headerWidget->setColumnWidth(role, width);
        m_view->applyColumnWidthsFromHeader();
    }
}

qreal KItemListHeader::columnWidth(const QByteArray& role) const
{
    return m_headerWidget->columnWidth(role);
}

void KItemListHeader::setColumnWidths(const QHash<QByteArray, qreal>& columnWidths)
{
    if (!m_headerWidget->automaticColumnResizing()) {
        foreach (const QByteArray& role, m_view->visibleRoles()) {
            const qreal width = columnWidths.value(role);
            m_headerWidget->setColumnWidth(role, width);
        }

        m_view->applyColumnWidthsFromHeader();
    }
}

KItemListHeader::KItemListHeader(KItemListView* listView) :
    QObject(listView->parent()),
    m_view(listView)
{
    m_headerWidget = m_view->m_headerWidget;
    Q_ASSERT(m_headerWidget);

    connect(m_headerWidget, SIGNAL(columnWidthChanged(QByteArray,qreal,qreal)),
            this, SIGNAL(columnWidthChanged(QByteArray,qreal,qreal)));
}

#include "kitemlistheader.moc"
