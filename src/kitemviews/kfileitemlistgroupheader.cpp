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

#include "kfileitemlistgroupheader.h"

#include <kratingpainter.h>
#include <QPainter>

KFileItemListGroupHeader::KFileItemListGroupHeader(QGraphicsWidget* parent) :
    KItemListGroupHeader(parent),
    m_dirtyCache(true),
    m_text(),
    m_pixmap()
{
    m_text.setTextFormat(Qt::PlainText);
    m_text.setPerformanceHint(QStaticText::AggressiveCaching);
}

KFileItemListGroupHeader::~KFileItemListGroupHeader()
{
}

void KFileItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    KItemListGroupHeader::paint(painter, option, widget);

    if (m_dirtyCache) {
        updateCache();
    }

    if (m_pixmap.isNull()) {
        painter->setPen(roleColor());
        painter->drawStaticText(roleBounds().topLeft(), m_text);
    } else {
        painter->drawPixmap(roleBounds().topLeft(), m_pixmap);
    }
}

void KFileItemListGroupHeader::roleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyCache = true;
}

void KFileItemListGroupHeader::dataChanged(const QVariant& current, const QVariant& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyCache = true;
}

void KFileItemListGroupHeader::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    m_dirtyCache = true;
}

void KFileItemListGroupHeader::updateCache()
{
    Q_ASSERT(m_dirtyCache);
    m_dirtyCache = false;

    const qreal maxWidth = size().width() - 4 * styleOption().padding;

    if (role() == "rating") {
        m_text = QString();

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

#include "kfileitemlistgroupheader.moc"
