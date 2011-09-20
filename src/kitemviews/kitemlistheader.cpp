/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kitemlistheader_p.h"

#include "kitemmodelbase.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QStyleOptionHeader>

KItemListHeader::KItemListHeader(QGraphicsWidget* parent) :
    QGraphicsWidget(parent),
    m_model(0),
    m_visibleRoles(),
    m_visibleRolesWidths()
{
    QStyleOptionHeader opt;
    const QSize headerSize = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize());
    resize(0, headerSize.height());
}

KItemListHeader::~KItemListHeader()
{
}

void KItemListHeader::setModel(KItemModelBase* model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                   this, SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
        disconnect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                   this, SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
    }

    m_model = model;

    if (m_model) {
        connect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                this, SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
        connect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                this, SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
    }
}

KItemModelBase* KItemListHeader::model() const
{
    return m_model;
}

void KItemListHeader::setVisibleRoles(const QList<QByteArray>& roles)
{
    m_visibleRoles = roles;
    update();
}

QList<QByteArray> KItemListHeader::visibleRoles() const
{
    return m_visibleRoles;
}

void KItemListHeader::setVisibleRolesWidths(const QHash<QByteArray, qreal> rolesWidths)
{
    m_visibleRolesWidths = rolesWidths;
    update();
}

QHash<QByteArray, qreal> KItemListHeader::visibleRolesWidths() const
{
    return m_visibleRolesWidths;
}

void KItemListHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Draw background
    QStyleOption opt;
    opt.init(widget);
    opt.rect = rect().toRect();
    opt.state |= QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, painter);

    if (!m_model) {
        return;
    }

    // Draw roles
    // TODO: This is a rough draft only
    QFontMetricsF fontMetrics(font());
    QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);

    painter->setFont(font());
    painter->setPen(palette().text().color());

    const qreal margin = style()->pixelMetric(QStyle::PM_HeaderMargin);
    qreal x = margin;
    foreach (const QByteArray& role, m_visibleRoles) {
        const QString roleDescription = m_model->roleDescription(role);
        const qreal textWidth = fontMetrics.width(roleDescription);
        QRectF rect(x, 0, textWidth, size().height());
        painter->drawText(rect, roleDescription, textOption);

        x += m_visibleRolesWidths.value(role) + margin;
    }
}

void KItemListHeader::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListHeader::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

#include "kitemlistheader_p.moc"
