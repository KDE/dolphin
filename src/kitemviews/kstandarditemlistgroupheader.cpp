/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kstandarditemlistgroupheader.h"

#include <KRatingPainter>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

KStandardItemListGroupHeader::KStandardItemListGroupHeader(QGraphicsWidget *parent)
    : KItemListGroupHeader(parent)
    , m_dirtyCache(true)
    , m_text()
    , m_pixmap()
{
}

KStandardItemListGroupHeader::~KStandardItemListGroupHeader() = default;

void KStandardItemListGroupHeader::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_dirtyCache) {
        updateCache();
    }
    KItemListGroupHeader::paint(painter, option, widget);
}

void KStandardItemListGroupHeader::paintRole(QPainter *painter, const QRectF &roleBounds, const QColor &color)
{
    const int arrowSize = qMax(8, styleOption().fontMetrics.height());
    const int arrowSpacing = qMax(2, styleOption().padding);
    const QRect arrowRect(qRound(roleBounds.left()), qRound(roleBounds.center().y() - arrowSize * 0.5), arrowSize, arrowSize);

    QStyle::PrimitiveElement pe;
    if (isCollapsed()) {
        pe = (layoutDirection() == Qt::RightToLeft) ? QStyle::PE_IndicatorArrowLeft : QStyle::PE_IndicatorArrowRight;
    } else {
        pe = QStyle::PE_IndicatorArrowDown;
    }

    QStyleOption opt;
    opt.rect = arrowRect;
    opt.state = QStyle::State_Enabled;
    opt.palette = QApplication::palette();
    opt.palette.setColor(QPalette::WindowText, color);
    opt.palette.setColor(QPalette::ButtonText, color);
    opt.palette.setColor(QPalette::Text, color);

    QApplication::style()->drawPrimitive(pe, &opt, painter, nullptr);

    const QRectF textBounds = roleBounds.adjusted(arrowSize + arrowSpacing, 0, 0, 0);
    painter->setPen(color);
    if (m_pixmap.isNull()) {
        painter->drawText(textBounds, 0, m_text);
    } else {
        painter->drawPixmap(textBounds.topLeft(), m_pixmap);
    }
}

void KStandardItemListGroupHeader::paintSeparator(QPainter *painter, const QColor &color)
{
    if (itemIndex() == 0) {
        // No top- or left-line should be drawn for the first group-header
        return;
    }

    painter->setPen(color);

    if (scrollOrientation() == Qt::Horizontal) {
        const qreal x = layoutDirection() == Qt::RightToLeft ? size().width() - 1 : 0;
        painter->drawLine(x, 0, x, size().height() - 1);
    } else {
        if (layoutDirection() == Qt::LeftToRight) {
            painter->drawLine(0, 0, size().width() - 1, 0);
        } else {
            painter->drawLine(1, 0, size().width(), 0);
        }
    }
}

void KStandardItemListGroupHeader::roleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::dataChanged(const QVariant &current, const QVariant &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::collapsedChanged(bool current, bool previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    KItemListGroupHeader::resizeEvent(event);
    m_dirtyCache = true;
}

void KStandardItemListGroupHeader::updateCache()
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
        m_text = text;
    }
}

#include "moc_kstandarditemlistgroupheader.cpp"
