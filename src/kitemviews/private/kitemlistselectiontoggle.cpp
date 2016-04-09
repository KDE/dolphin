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

#include "kitemlistselectiontoggle.h"

#include <KIconLoader>
#include <QPainter>


KItemListSelectionToggle::KItemListSelectionToggle(QGraphicsItem* parent) :
    QGraphicsWidget(parent, 0),
    m_checked(false),
    m_hovered(false)
{
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

void KItemListSelectionToggle::setHovered(bool hovered)
{
    if (m_hovered != hovered) {
        m_hovered = hovered;
        m_pixmap = QPixmap();
        update();
    }
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

void KItemListSelectionToggle::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);

    if (!m_pixmap.isNull()) {
        const int pixmapSize = m_pixmap.size().width(); // Pixmap width is always equal pixmap height

        if (pixmapSize != iconSize()) {
            // If the required icon size is different from the actual pixmap size,
            // overwrite the m_pixmap with an empty pixmap and reload the new
            // icon on next re-painting.
            m_pixmap = QPixmap();
        }
    }
}

void KItemListSelectionToggle::updatePixmap()
{
    const QString icon = m_checked ? QStringLiteral("emblem-remove") : QStringLiteral("emblem-added");
    const KIconLoader::States state = m_hovered ? KIconLoader::ActiveState : KIconLoader::DisabledState;
    m_pixmap = KIconLoader::global()->loadIcon(icon, KIconLoader::Desktop, iconSize(), state);
}

int KItemListSelectionToggle::iconSize() const
{
    const int iconSize = qMin(size().width(), size().height());

    if (iconSize < KIconLoader::SizeSmallMedium) {
        return KIconLoader::SizeSmall;
    } else if (iconSize < KIconLoader::SizeMedium) {
        return KIconLoader::SizeSmallMedium;
    } else if (iconSize < KIconLoader::SizeLarge) {
        return KIconLoader::SizeMedium;
    } else if (iconSize < KIconLoader::SizeHuge) {
        return KIconLoader::SizeLarge;
    } else if (iconSize < KIconLoader::SizeEnormous) {
        return KIconLoader::SizeHuge;
    }

    return iconSize;
}

