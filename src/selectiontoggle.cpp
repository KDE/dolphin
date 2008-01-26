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

#include <kdebug.h>

SelectionToggle::SelectionToggle(QWidget* parent) :
    QAbstractButton(parent),
    m_showIcon(false),
    m_isHovered(false),
    m_icon(),
    m_timer(0)
{
    parent->installEventFilter(this);
    resize(sizeHint());
    m_icon = KIconLoader::global()->loadIcon("dialog-ok",
                                             KIconLoader::NoGroup,
                                             KIconLoader::SizeSmall);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(showIcon()));
}

SelectionToggle::~SelectionToggle()
{
}

QSize SelectionToggle::sizeHint() const
{
    return QSize(16, 16);
}

void SelectionToggle::setVisible(bool visible)
{
    QAbstractButton::setVisible(visible);
    if (visible) {
        m_timer->start(1000);
    } else {
        m_timer->stop();
        m_showIcon = false;
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
    m_isHovered = true;
    m_showIcon = true;
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
    } else if (m_showIcon) {
        painter.drawPixmap(0, 0, m_icon);
    }
}

void SelectionToggle::showIcon()
{
    m_showIcon = true;
    update();
}

#include "selectiontoggle.moc"
