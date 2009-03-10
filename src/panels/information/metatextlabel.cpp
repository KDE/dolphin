/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "metatextlabel.h"

#include <kglobalsettings.h>
#include <klocale.h>

#include <QPainter>
#include <QTextLayout>
#include <QTextLine>
#include <kdebug.h>

MetaTextLabel::MetaTextLabel(QWidget* parent) :
    QWidget(parent),
    m_metaInfos()
{
    setFont(KGlobalSettings::smallestReadableFont());
    setMinimumHeight(0);
}

MetaTextLabel::~MetaTextLabel()
{
}

void MetaTextLabel::clear()
{
    setMinimumHeight(0);
    m_metaInfos.clear();
    update();
}

void MetaTextLabel::add(const QString& labelText, const QString& infoText)
{
    MetaInfo metaInfo;
    metaInfo.label = labelText;
    metaInfo.info = infoText;

    // add the meta information alphabetically sorted
    bool inserted = false;
    const int count = m_metaInfos.size();
    for (int i = 0; i < count; ++i) {
        if (m_metaInfos[i].label > labelText) {
            m_metaInfos.insert(i, metaInfo);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        m_metaInfos.append(metaInfo);
    }

    setMinimumHeight(minimumHeight() + requiredHeight(metaInfo));
    update();
}

void MetaTextLabel::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    const QColor infoColor = palette().color(QPalette::Foreground);
    QColor labelColor = infoColor;
    labelColor.setAlpha(128);

    int y = 0;
    const int infoWidth = width() / 2;
    const int labelWidth = infoWidth - 2 * Spacing;
    const int infoX = infoWidth;
    const int maxHeight = maxHeightPerLine();

    foreach (const MetaInfo& metaInfo, m_metaInfos) {
        // draw label (e. g. "Date:")
        painter.setPen(labelColor);
        painter.drawText(0, y, labelWidth, maxHeight,
                         Qt::AlignTop | Qt::AlignRight | Qt::TextWordWrap,
                         metaInfo.label);

        // draw information (e. g. "2008-11-09 20:12")
        painter.setPen(infoColor);
        painter.drawText(infoX, y, infoWidth, maxHeight,
                         Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
                         metaInfo.info);

        y += requiredHeight(metaInfo);
    }
}

void MetaTextLabel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    int minimumHeight = 0;
    foreach (const MetaInfo& metaInfo, m_metaInfos) {
        minimumHeight += requiredHeight(metaInfo);
    }
    setMinimumHeight(minimumHeight);
}

int MetaTextLabel::requiredHeight(const MetaInfo& metaInfo) const
{
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);

    qreal height = 0;
    const int leading = fontMetrics().leading();
    const int availableWidth = width() / 2;

    QTextLayout textLayout(metaInfo.info);
    textLayout.setFont(font());
    textLayout.setTextOption(textOption);

    textLayout.beginLayout();
    QTextLine line = textLayout.createLine();
    while (line.isValid()) {
        line.setLineWidth(availableWidth);
        height += leading;
        height += line.height();
        line = textLayout.createLine();
    }
    textLayout.endLayout();

    int adjustedHeight = static_cast<int>(height);
    if (adjustedHeight > maxHeightPerLine()) {
        adjustedHeight = maxHeightPerLine();
    }

    return adjustedHeight + Spacing;
}

int MetaTextLabel::maxHeightPerLine() const
{
    return fontMetrics().height() * 100;
}

#include "metatextlabel.moc"
