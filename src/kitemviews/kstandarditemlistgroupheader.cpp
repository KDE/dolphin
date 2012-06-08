/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kstandarditemlistgroupheader.h"

#include <kratingpainter.h>
#include <QPainter>

KStandardItemListGroupHeader::KStandardItemListGroupHeader(QGraphicsWidget* parent) :
    KItemListGroupHeader(parent),
    m_dirtyCache(true),
    m_text(),
    m_pixmap()
{
    m_text.setTextFormat(Qt::PlainText);
    m_text.setPerformanceHint(QStaticText::AggressiveCaching);
}

KStandardItemListGroupHeader::~KStandardItemListGroupHeader()
{
}

void KStandardItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (m_dirtyCache) {
        updateCache();
    }
    KItemListGroupHeader::paint(painter, option, widget);
}

void KStandardItemListGroupHeader::paintRole(QPainter* painter, const QRectF& roleBounds, const QColor& color)
{
    if (m_pixmap.isNull()) {
        painter->setPen(color);
        painter->drawStaticText(roleBounds.topLeft(), m_text);
    } else {
        painter->drawPixmap(roleBounds.topLeft(), m_pixmap);
    }
}

void KStandardItemListGroupHeader::paintSeparator(QPainter* painter, const QColor& color)
{
    if (itemIndex() == 0) {
        // No top- or left-line should be drawn for the first group-header
        return;
    }

    painter->setPen(color);

    if (scrollOrientation() == Qt::Horizontal) {
        painter->drawLine(0, 0, 0, size().height() - 1);
    } else {
        painter->drawLine(0, 0, size().width() - 1, 0);
    }
}

void KStandardItemListGroupHeader::roleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::dataChanged(const QVariant& current, const QVariant& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::updateCache()
{
    Q_ASSERT(m_dirtyCache);
    m_dirtyCache = false;

    const qreal maxWidth = size().width() - 4 * styleOption().padding;

    if (role() == "rating") {
        m_text = QString(); // krazy:exlude=nullstrassign

        const qreal height = styleOption().fontMetrics.ascent();
        const QSizeF pixmapSize(qMin(height * 5, maxWidth), height);

        m_pixmap = QPixmap(pixmapSize.toSize());
        m_pixmap.fill(Qt::transparent);

        QPainter painter(&m_pixmap);
        const QRect rect(0, 0, m_pixmap.width(), m_pixmap.height());
        const int rating = data().toInt();
        KRatingPainter::paintRating(&painter, rect, Qt::AlignJustify | Qt::AlignVCenter, rating);
    } else {
        m_pixmap = QPixmap();

        QFontMetricsF fontMetrics(font());
        const QString text = fontMetrics.elidedText(data().toString(), Qt::ElideRight, maxWidth);
        m_text.setText(text);
    }
}

#include "kstandarditemlistgroupheader.moc"
