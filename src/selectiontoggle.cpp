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

#include "selectiontoggle.h"

#include <kicon.h>
#include <kiconloader.h>
#include <kiconeffect.h>

#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QTimer>
#include <QTimeLine>

SelectionToggle::SelectionToggle(QWidget* parent) :
    QAbstractButton(parent),
    m_isHovered(false),
    m_fadingValue(0),
    m_icon(),
    m_fadingTimeLine(0)
{
    parent->installEventFilter(this);
    resize(sizeHint());
    m_icon = KIconLoader::global()->loadIcon("dialog-ok",
                                             KIconLoader::NoGroup,
                                             KIconLoader::SizeSmall);
}

SelectionToggle::~SelectionToggle()
{
}

QSize SelectionToggle::sizeHint() const
{
    return QSize(16, 16);
}

void SelectionToggle::reset()
{
    m_item = KFileItem();
    hide();
}

void SelectionToggle::setFileItem(const KFileItem& item)
{
    m_item = item;
    if (!item.isNull()) {
        startFading();
    }
}

KFileItem SelectionToggle::fileItem() const
{
    return m_item;
}

void SelectionToggle::setVisible(bool visible)
{
    QAbstractButton::setVisible(visible);

    stopFading();
    if (visible) {
        startFading();
    }

}

bool SelectionToggle::eventFilter(QObject* obj, QEvent* event)
{
    if ((obj == parent()) && (event->type() == QEvent::Leave)) {
        hide();
    }
    return QAbstractButton::eventFilter(obj, event);
}

void SelectionToggle::enterEvent(QEvent* event)
{
    QAbstractButton::enterEvent(event);

    // if the mouse cursor is above the selection toggle, display
    // it immediately without fading timer
    m_isHovered = true;
    if (m_fadingTimeLine != 0) {
        m_fadingTimeLine->stop();
    }
    m_fadingValue = 255;
    update();
}

void SelectionToggle::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    m_isHovered = false;
    update();
}

void SelectionToggle::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setClipRect(event->rect());

    if (m_isHovered) {
        KIconEffect iconEffect;
        QPixmap activeIcon = iconEffect.apply(m_icon, KIconLoader::Desktop, KIconLoader::ActiveState);
        painter.drawPixmap(0, 0, activeIcon);
    } else {
        if (m_fadingValue < 255) {
            // apply an alpha mask respecting the fading value to the icon
            QPixmap icon = m_icon;
            QPixmap alphaMask(icon.width(), icon.height());
            const QColor color(m_fadingValue, m_fadingValue, m_fadingValue);
            alphaMask.fill(color);
            icon.setAlphaChannel(alphaMask);
            painter.drawPixmap(0, 0, icon);
        } else {
            // no fading is required
            painter.drawPixmap(0, 0, m_icon);
        }
    }
}

void SelectionToggle::setFadingValue(int value)
{
    m_fadingValue = value;
    if (m_fadingValue >= 255) {
        Q_ASSERT(m_fadingTimeLine != 0);
        m_fadingTimeLine->stop();
    }
    update();
}

void SelectionToggle::startFading()
{
    Q_ASSERT(m_fadingTimeLine == 0);

    m_fadingTimeLine = new QTimeLine(2000, this);
    connect(m_fadingTimeLine, SIGNAL(frameChanged(int)),
            this, SLOT(setFadingValue(int)));
    m_fadingTimeLine->setFrameRange(0, 255);
    m_fadingTimeLine->start();
    m_fadingValue = 0;
}

void SelectionToggle::stopFading()
{
    if (m_fadingTimeLine != 0) {
        m_fadingTimeLine->stop();
        delete m_fadingTimeLine;
        m_fadingTimeLine = 0;
    }
    m_fadingValue = 0;
}

#include "selectiontoggle.moc"
