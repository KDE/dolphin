/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "statusbarmessagelabel.h"

#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kicon.h>
#include <klocale.h>

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>

StatusBarMessageLabel::StatusBarMessageLabel(QWidget* parent) :
    QWidget(parent),
    m_type(DolphinStatusBar::Default),
    m_state(Default),
    m_illumination(0),
    m_minTextHeight(-1),
    m_timer(0),
    m_closeButton(0)
{
    setMinimumHeight(K3Icon::SizeSmall);

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(timerDone()));

    m_closeButton = new QPushButton(KIcon("close"), QString::null, this);
    m_closeButton->hide();
}

StatusBarMessageLabel::~StatusBarMessageLabel()
{
}

void StatusBarMessageLabel::setType(DolphinStatusBar::Type type)
{
    if ((type != m_type) || (type == DolphinStatusBar::Error)) {
        m_type = type;

        m_timer->stop();
        m_illumination = 0;
        m_state = Default;

        const char* iconName = 0;
        QPixmap pixmap;
        switch (type) {
            case DolphinStatusBar::OperationCompleted:
                iconName = "ok";
                m_closeButton->hide();
                break;

            case DolphinStatusBar::Information:
                iconName = "info";
                m_closeButton->hide();
                break;

            case DolphinStatusBar::Error:
                iconName = "error";
                m_timer->start(100);
                m_state = Illuminate;

                updateCloseButtonPosition();
                m_closeButton->show();
                break;

            case DolphinStatusBar::Default:
            default:
                m_closeButton->hide();
                break;
        }

        m_pixmap = (iconName == 0) ? QPixmap() : SmallIcon(iconName);
        QTimer::singleShot(GeometryTimeout, this, SLOT(assureVisibleText()));
        update();
    }
}

void StatusBarMessageLabel::setText(const QString& text)
{
    if (text != m_text) {
        if (m_type == DolphinStatusBar::Error) {
            m_timer->start(100);
            m_illumination = 0;
            m_state = Illuminate;
        }
        m_text = text;
        QTimer::singleShot(GeometryTimeout, this, SLOT(assureVisibleText()));
        update();
    }
}

void StatusBarMessageLabel::setMinimumTextHeight(int min)
{
    if (min != m_minTextHeight) {
        m_minTextHeight = min;
        setMinimumHeight(min);
    }
}

int StatusBarMessageLabel::widthGap() const
{
    QFontMetrics fontMetrics(font());
    const int defaultGap = 10;
    return fontMetrics.width(m_text) - availableTextWidth() + defaultGap;
}

void StatusBarMessageLabel::paintEvent(QPaintEvent* /* event */)
{
    QPainter painter(this);

    // draw background
    QColor backgroundColor(palette().brush(QPalette::Background).color());
    QColor foregroundColor(KGlobalSettings::textColor());
    if (m_illumination > 0) {
        backgroundColor = mixColors(backgroundColor, QColor(255, 255, 128), m_illumination);
        foregroundColor = mixColors(foregroundColor, QColor(0, 0, 0), m_illumination);
    }
    painter.setBrush(backgroundColor);
    painter.setPen(backgroundColor);
    painter.drawRect(QRect(0, 0, width(), height()));

    // draw pixmap
    int x = borderGap();
    int y = (m_minTextHeight - m_pixmap.height()) / 2;

    if (!m_pixmap.isNull()) {
        painter.drawPixmap(x, y, m_pixmap);
        x += m_pixmap.width() + borderGap();
    }

    // draw text
    painter.setPen(foregroundColor);
    int flags = Qt::AlignVCenter;
    if (height() > m_minTextHeight) {
        flags = flags | Qt::TextWordWrap;
    }
    painter.drawText(QRect(x, 0, availableTextWidth(), height()), flags, m_text);
    painter.end();
}

void StatusBarMessageLabel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(GeometryTimeout, this, SLOT(assureVisibleText()));
}

void StatusBarMessageLabel::timerDone()
{
    switch (m_state) {
        case Illuminate: {
            // increase the illumination
            if (m_illumination < 100) {
                m_illumination += 20;
                update();
            }
            else {
                m_state = Illuminated;
                m_timer->start(5000);
            }
            break;
        }

        case Illuminated: {
            // start desaturation
            m_state = Desaturate;
            m_timer->start(100);
            break;
        }

        case Desaturate: {
            // desaturate
            if (m_illumination > 0) {
                m_illumination -= 5;
                update();
            }
            else {
                m_state = Default;
                m_timer->stop();
            }
            break;
        }

        default:
            break;
    }
}

void StatusBarMessageLabel::assureVisibleText()
{
    if (m_text.isEmpty()) {
        return;
    }

    // calculate the required height of the widget thats
    // needed for having a fully visible text
    QFontMetrics fontMetrics(font());
    const QRect bounds(fontMetrics.boundingRect(0, 0, availableTextWidth(), height(),
                                                Qt::AlignVCenter | Qt::TextWordWrap,
                                                m_text));
    int requiredHeight = bounds.height();
    if (requiredHeight < m_minTextHeight) {
        requiredHeight = m_minTextHeight;
    }

    // Increase/decrease the current height of the widget to the
    // required height. The increasing/decreasing is done in several
    // steps to have an animation if the height is modified
    // (see StatusBarMessageLabel::resizeEvent())
    const int gap = m_minTextHeight / 2;
    int minHeight = minimumHeight();
    if (minHeight < requiredHeight) {
        minHeight += gap;
        if (minHeight > requiredHeight) {
            minHeight = requiredHeight;
        }
        setMinimumHeight(minHeight);
        updateGeometry();
    }
    else if (minHeight > requiredHeight) {
        minHeight -= gap;
        if (minHeight < requiredHeight) {
            minHeight = requiredHeight;
        }
        setMinimumHeight(minHeight);
        updateGeometry();
    }

    updateCloseButtonPosition();
}

int StatusBarMessageLabel::availableTextWidth() const
{
    const int buttonWidth = (m_type == DolphinStatusBar::Error) ?
                            m_closeButton->width() + borderGap() : 0;
    return width() - m_pixmap.width() - (borderGap() * 3) - buttonWidth;
}

QColor StatusBarMessageLabel::mixColors(const QColor& c1,
                                        const QColor& c2,
                                        int percent) const
{
    const int recip = 100 - percent;
    const int red   = (c1.red()   * recip + c2.red()   * percent) / 100;
    const int green = (c1.green() * recip + c2.green() * percent) / 100;
    const int blue  = (c1.blue()  * recip + c2.blue()  * percent) / 100;
    return QColor(red, green, blue);
}

void StatusBarMessageLabel::updateCloseButtonPosition()
{
    const int x = width() - m_closeButton->width();
    const int y = 2;
    m_closeButton->move(x, y);
}

#include "statusbarmessagelabel.moc"
