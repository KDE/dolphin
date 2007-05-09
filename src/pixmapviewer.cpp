/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "pixmapviewer.h"

#include <kiconloader.h>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QKeyEvent>

PixmapViewer::PixmapViewer(QWidget* parent, Transition transition) :
    QWidget(parent),
    m_transition(transition),
    m_animationStep(0)
{
    setMinimumWidth(K3Icon::SizeEnormous);
    setMinimumWidth(K3Icon::SizeEnormous);

    m_animation.setDuration(300);
    connect(&m_animation, SIGNAL(valueChanged(qreal)), this, SLOT(update()));
}

PixmapViewer::~PixmapViewer()
{
}

void PixmapViewer::setPixmap(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        return;
    }

    m_oldPixmap = m_pixmap.isNull() ? pixmap : m_pixmap;
    m_pixmap = pixmap;
    update();

    const bool animate = (m_transition != NoTransition) &&
                         (m_pixmap.size() != m_oldPixmap.size());
    if (animate) {
        m_animation.start();
    }
}

void PixmapViewer::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    const float value = m_animation.currentValue();

    const int scaledWidth  = static_cast<int>((m_oldPixmap.width()  * (1.0 - value)) + (m_pixmap.width()  * value));
    const int scaledHeight = static_cast<int>((m_oldPixmap.height() * (1.0 - value)) + (m_pixmap.height() * value));
    const int x = (width()  - scaledWidth ) / 2;
    const int y = (height() - scaledHeight) / 2;

    const bool useOldPixmap = (m_transition == SizeTransition) &&
                              (m_oldPixmap.width() > m_pixmap.width());
    const QPixmap& largePixmap = useOldPixmap ? m_oldPixmap : m_pixmap;
    const QPixmap scaledPixmap = largePixmap.scaled(scaledWidth,
                                                    scaledHeight,
                                                    Qt::IgnoreAspectRatio,
                                                    Qt::SmoothTransformation);
    painter.drawPixmap(x, y, scaledPixmap);
}

#include "pixmapviewer.moc"
