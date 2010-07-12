/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
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

#include "filemetadatatooltip.h"

#include <kcolorscheme.h>
#include <kfilemetadatawidget.h>
#include <kseparator.h>
#include <kwindowsystem.h>

#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

FileMetaDataToolTip::FileMetaDataToolTip(QWidget* parent) :
    QWidget(parent),
    m_preview(0),
    m_name(0),
    m_fileMetaDataWidget(0)

{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

    // Create widget for file preview
    m_preview = new QLabel(this);

    // Create widget for file name
    m_name = new QLabel(this);
    QFont font = m_name->font();
    font.setBold(true);
    m_name->setFont(font);
    m_name->setAlignment(Qt::AlignHCenter);

    // Create widget for the meta data
    m_fileMetaDataWidget = new KFileMetaDataWidget();
    m_fileMetaDataWidget->setForegroundRole(QPalette::ToolTipText);
    m_fileMetaDataWidget->setReadOnly(true);

    // The stretchwidget allows the metadata widget to be top aligned and fills
    // the remaining vertical space
    QWidget* stretchWidget = new QWidget(this);
    stretchWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    QWidget* textContainer = new QWidget(this);
    QVBoxLayout* textLayout = new QVBoxLayout(textContainer);
    textLayout->addWidget(m_name);
    textLayout->addWidget(new KSeparator());
    textLayout->addWidget(m_fileMetaDataWidget);
    textLayout->addWidget(stretchWidget);

    QHBoxLayout* tipLayout = new QHBoxLayout(this);
    tipLayout->addWidget(m_preview);
    tipLayout->addWidget(textContainer);
    
    tipLayout->setSizeConstraint(QLayout::SetFixedSize);
}

FileMetaDataToolTip::~FileMetaDataToolTip()
{
}

void FileMetaDataToolTip::setPreview(const QPixmap& pixmap)
{
    m_preview->setPixmap(pixmap);
}

const QPixmap* FileMetaDataToolTip::preview() const
{
    return m_preview->pixmap();
}

void FileMetaDataToolTip::setName(const QString& name)
{
    m_name->setText(name);
}

QString FileMetaDataToolTip::name() const
{
    return m_name->text();
}

void FileMetaDataToolTip::setItems(const KFileItemList& items)
{
    m_fileMetaDataWidget->setItems(items);
}

KFileItemList FileMetaDataToolTip::items() const
{
    return m_fileMetaDataWidget->items();
}

void FileMetaDataToolTip::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QColor toColor = palette().brush(QPalette::ToolTipBase).color();
    QColor fromColor = KColorScheme::shade(toColor, KColorScheme::LightShade, 0.2);

    const bool haveAlphaChannel = KWindowSystem::compositingActive();
    if (haveAlphaChannel) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(0.5, 0.5);
        toColor.setAlpha(220);
        fromColor.setAlpha(220);
    }

    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(0.0, height()));
    gradient.setColorAt(0.0, fromColor);
    gradient.setColorAt(1.0, toColor);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);

    const QRect rect(0, 0, width(), height());
    if (haveAlphaChannel) {
        const qreal radius = 5.0;

        QPainterPath path;
        path.moveTo(rect.left(), rect.top() + radius);
        arc(path, rect.left()  + radius, rect.top()    + radius, radius, 180, -90);
        arc(path, rect.right() - radius, rect.top()    + radius, radius,  90, -90);
        arc(path, rect.right() - radius, rect.bottom() - radius, radius,   0, -90);
        arc(path, rect.left()  + radius, rect.bottom() - radius, radius, 270, -90);
        path.closeSubpath();

        painter.drawPath(path);
    } else {
        painter.drawRect(rect);
    }
}

void FileMetaDataToolTip::arc(QPainterPath& path,
                              qreal cx, qreal cy,
                              qreal radius, qreal angle,
                              qreal sweepLength)
{
    path.arcTo(cx-radius, cy-radius, radius * 2, radius * 2, angle, sweepLength);
}

#include "filemetadatatooltip.moc"
