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

#include <kcolorscheme.h>
#include <kiconloader.h>
#include <kicon.h>
#include <klocale.h>

#include <QFontMetrics>
#include <QPainter>
#include <QKeyEvent>
#include <QPushButton>
#include <QPixmap>
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
    setMinimumHeight(KIconLoader::SizeSmall);

    QPalette palette;
    palette.setColor(QPalette::Background, Qt::transparent);
    setPalette(palette);

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(timerDone()));

    m_closeButton = new QPushButton(i18nc("@action:button", "Close"), this);
    m_closeButton->hide();
    connect(m_closeButton, SIGNAL(clicked()),
            this, SLOT(closeErrorMessage()));
}

StatusBarMessageLabel::~StatusBarMessageLabel()
{
}

void StatusBarMessageLabel::setMessage(const QString& text,
                                       DolphinStatusBar::Type type)
{
    if ((text == m_text) && (type == m_type)) {
        return;
    }

    if (m_type == DolphinStatusBar::Error) {
        if (type == DolphinStatusBar::Error) {
            m_pendingMessages.insert(0, m_text);
        } else if ((m_state != Default) || !m_pendingMessages.isEmpty()) {
            // a non-error message should not be shown, as there
            // are other pending error messages in the queue
            return;
        }
    }

    m_text = text;
    m_type = type;

    m_timer->stop();
    m_illumination = 0;
    m_state = Default;

    const char* iconName = 0;
    QPixmap pixmap;
    switch (type) {
    case DolphinStatusBar::OperationCompleted:
        iconName = "dialog-ok";
        // "ok" icon should probably be "dialog-success", but we don't have that icon in KDE 4.0
        m_closeButton->hide();
        break;

    case DolphinStatusBar::Information:
        iconName = "dialog-information";
        m_closeButton->hide();
        break;

    case DolphinStatusBar::Error:
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

void StatusBarMessageLabel::setMinimumTextHeight(int min)
{
    if (min != m_minTextHeight) {
        m_minTextHeight = min;
        setMinimumHeight(min);
        if (m_closeButton->height() > min) {
            m_closeButton->setFixedHeight(min);
        }
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
    QColor backgroundColor = palette().window().color();
    if (m_illumination > 0) {
        // at this point, a: we are a second label being drawn over the already
        // painted status area, so we can be translucent, and b: our palette's
        // window color (bg only) seems to be wrong (always black)
        KColorScheme scheme(palette().currentColorGroup(), KColorScheme::Window);
        backgroundColor = scheme.background(KColorScheme::NegativeBackground).color();
        backgroundColor.setAlpha(qMin(255, m_illumination*2));
    }
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(0, 0, width(), height()));

    // draw pixmap
    int x = BorderGap;
    int y = (m_minTextHeight - m_pixmap.height()) / 2;

    if (!m_pixmap.isNull()) {
        painter.drawPixmap(x, y, m_pixmap);
        x += m_pixmap.width() + BorderGap;
    }

    // draw text
    painter.setPen(palette().windowText().color());
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
    updateCloseButtonPosition();
    QTimer::singleShot(GeometryTimeout, this, SLOT(assureVisibleText()));
}

void StatusBarMessageLabel::timerDone()
{
    switch (m_state) {
    case Illuminate: {
        // increase the illumination
        const int illumination_max = 128;
        if (m_illumination < illumination_max) {
            m_illumination += 32;
            if (m_illumination > illumination_max) {
                m_illumination = illumination_max;
            }
            update();
        } else {
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
        } else {
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
    } else if (minHeight > requiredHeight) {
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
                            m_closeButton->width() + BorderGap : 0;
    return width() - m_pixmap.width() - (BorderGap * 4) - buttonWidth;
}

void StatusBarMessageLabel::updateCloseButtonPosition()
{
    const int x = width() - m_closeButton->width() - BorderGap;
    const int y = (height() - m_closeButton->height()) / 2;
    m_closeButton->move(x, y);
}

void StatusBarMessageLabel::closeErrorMessage()
{
    if (!showPendingMessage()) {
        reset();
        setMessage(m_defaultText, DolphinStatusBar::Default);
    }
}

bool StatusBarMessageLabel::showPendingMessage()
{
    if (!m_pendingMessages.isEmpty()) {
        reset();
        setMessage(m_pendingMessages.takeFirst(), DolphinStatusBar::Error);
        return true;
    }
    return false;
}

void StatusBarMessageLabel::reset()
{
    m_text.clear();
    m_type = DolphinStatusBar::Default;
}

#include "statusbarmessagelabel.moc"
