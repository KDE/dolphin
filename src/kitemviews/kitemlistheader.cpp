/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistheader.h"
#include "kitemlistview.h"
#include "private/kitemlistheaderwidget.h"

KItemListHeader::~KItemListHeader()
{
}

void KItemListHeader::setAutomaticColumnResizing(bool automatic)
{
    if (m_headerWidget->automaticColumnResizing() != automatic) {
        m_headerWidget->setAutomaticColumnResizing(automatic);
        if (automatic) {
            m_view->applyAutomaticColumnWidths();
            m_view->doLayout(KItemListView::NoAnimation);
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
        m_view->doLayout(KItemListView::NoAnimation);
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
        m_view->doLayout(KItemListView::NoAnimation);
    }
}

qreal KItemListHeader::preferredColumnWidth(const QByteArray& role) const
{
    return m_headerWidget->preferredColumnWidth(role);
}

KItemListHeader::KItemListHeader(KItemListView* listView) :
    QObject(listView),
    m_view(listView)
{
    m_headerWidget = m_view->m_headerWidget;
    Q_ASSERT(m_headerWidget);

    connect(m_headerWidget, &KItemListHeaderWidget::columnWidthChanged,
            this, &KItemListHeader::columnWidthChanged);
    connect(m_headerWidget, &KItemListHeaderWidget::columnWidthChangeFinished,
            this, &KItemListHeader::columnWidthChangeFinished);
}

