/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistselectiontoggle.h"

#include <KIconLoader>

#include <QIcon>
#include <QPainter>

KItemListSelectionToggle::KItemListSelectionToggle(QGraphicsItem* parent) :
    QGraphicsWidget(parent),
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
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (m_pixmap.isNull()) {
        updatePixmap();
    }

    const qreal x = (size().width()  - qreal(m_pixmap.width() / m_pixmap.devicePixelRatioF()))  / 2;
    const qreal y = (size().height() - qreal(m_pixmap.height() / m_pixmap.devicePixelRatioF())) / 2;
    painter->drawPixmap(x, y, m_pixmap);
}

void KItemListSelectionToggle::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);

    if (!m_pixmap.isNull()) {
        const int pixmapSize = m_pixmap.size().width() / m_pixmap.devicePixelRatioF(); // Pixmap width is always equal pixmap height

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
    m_pixmap = QIcon::fromTheme(icon).pixmap(iconSize(), m_hovered ? QIcon::Active : QIcon::Disabled);
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

