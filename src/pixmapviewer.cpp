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

#include <kglobalsettings.h>
#include <kiconloader.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <QPaintEvent>

PixmapViewer::PixmapViewer(QWidget* parent) :
        QWidget(parent)
{
    setMinimumWidth(K3Icon::SizeEnormous);
    setMinimumWidth(K3Icon::SizeEnormous);
}

PixmapViewer::~PixmapViewer()
{}

void PixmapViewer::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    update();
}

void PixmapViewer::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter;
    painter.begin(this);
    const int x = (width() - m_pixmap.width()) / 2;
    const int y = (height() - m_pixmap.height()) / 2;
    painter.drawPixmap(x, y, m_pixmap);
    painter.end();
}

#include "pixmapviewer.moc"
