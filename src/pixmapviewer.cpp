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

#include "pixmapviewer.h"

#include <kiconloader.h>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPaintEvent>
#include <QtDebug>

PixmapViewer::PixmapViewer(QWidget* parent) :
    QWidget(parent)
   ,m_animationStep(0)
{
    setMinimumWidth(K3Icon::SizeEnormous);
    setMinimumWidth(K3Icon::SizeEnormous);

    static const int ANIMATION_DURATION = 750;
    m_animation.setDuration(ANIMATION_DURATION);

    connect( &m_animation , SIGNAL(valueChanged(qreal)) , this , SLOT(update()) );
    connect( &m_animation , SIGNAL(finished()) , this , SLOT(finishTransition()) );
}

PixmapViewer::~PixmapViewer()
{
}

void PixmapViewer::setPixmap(const QPixmap& pixmap)
{
    if ( pixmap.isNull() )
        return;

    m_pendingPixmap = pixmap;

    if ( m_animation.state() == QTimeLine::NotRunning )
        beginTransition();
}

void PixmapViewer::beginTransition()
{
    Q_ASSERT( !m_pendingPixmap.isNull() );
    Q_ASSERT( m_nextPixmap.isNull() );

    m_nextPixmap = m_pendingPixmap;
    m_pendingPixmap = QPixmap();
    m_animation.start(); 
}

void PixmapViewer::finishTransition()
{
    m_pixmap = m_nextPixmap;
    m_nextPixmap = QPixmap();

    if ( !m_pendingPixmap.isNull() )
    {
        beginTransition();
    }
}

void PixmapViewer::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter;
    painter.begin(this);
    const int x = (width() - m_pixmap.width()) / 2;
    const int y = (height() - m_pixmap.height()) / 2;

    if ( !m_nextPixmap.isNull() )
    {
        const int nextPixmapX = (width() - m_nextPixmap.width()) / 2;
        const int nextPixmapY = (height() - m_nextPixmap.height()) / 2;

        painter.setOpacity( 1 - m_animation.currentValue() );
        painter.drawPixmap(x, y, m_pixmap);
        painter.setOpacity( m_animation.currentValue() );
        painter.drawPixmap(nextPixmapX,nextPixmapY,m_nextPixmap);
    }
    else
    {
        painter.drawPixmap(x, y, m_pixmap);
    }

    painter.end();
}

#include "pixmapviewer.moc"
