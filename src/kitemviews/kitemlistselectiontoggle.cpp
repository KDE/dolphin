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

#include "kitemlistselectiontoggle_p.h"

#include <KIconEffect>
#include <KIconLoader>
#include <QPainter>

#include <KDebug>

KItemListSelectionToggle::KItemListSelectionToggle(QGraphicsItem* parent) :
    QGraphicsWidget(parent, 0),
    m_checked(false),
    m_hovered(false)
{
    setAcceptHoverEvents(true);
}

KItemListSelectionToggle::~KItemListSelectionToggle()
{
}

void KItemListSelectionToggle::setChecked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        m_pixmap = QPixmap();
        update();
    }
}

bool KItemListSelectionToggle::isChecked() const
{
    return m_checked;
}

void KItemListSelectionToggle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_pixmap.isNull()) {
        updatePixmap();
    }

    const qreal x = (size().width()  - qreal(m_pixmap.width()))  / 2;
    const qreal y = (size().height() - qreal(m_pixmap.height())) / 2;
    painter->drawPixmap(x, y, m_pixmap);
}

void KItemListSelectionToggle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverEnterEvent(event);
    m_hovered = true;
    m_pixmap = QPixmap();
}

void KItemListSelectionToggle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverLeaveEvent(event);
    m_hovered = false;
    m_pixmap = QPixmap();
}

void KItemListSelectionToggle::updatePixmap()
{
    const char* icon = m_checked ? "list-remove" : "list-add";

    int iconSize = qMin(size().width(), size().height());
    if (iconSize < KIconLoader::SizeSmallMedium) {
        iconSize = KIconLoader::SizeSmall;
    } else if (iconSize < KIconLoader::SizeMedium) {
        iconSize = KIconLoader::SizeSmallMedium;
    } else if (iconSize < KIconLoader::SizeLarge) {
        iconSize = KIconLoader::SizeMedium;
    } else if (iconSize < KIconLoader::SizeHuge) {
        iconSize = KIconLoader::SizeLarge;
    } else if (iconSize < KIconLoader::SizeEnormous) {
        iconSize = KIconLoader::SizeHuge;
    }

    m_pixmap = KIconLoader::global()->loadIcon(QLatin1String(icon), KIconLoader::NoGroup, iconSize);

    if (m_hovered) {
        KIconLoader::global()->iconEffect()->apply(m_pixmap, KIconLoader::Desktop, KIconLoader::ActiveState);
    }
}

#include "kitemlistselectiontoggle_p.moc"
