/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kfileitemlistgroupheader.h"

#include <QPainter>

KFileItemListGroupHeader::KFileItemListGroupHeader(QGraphicsWidget* parent) :
    KItemListGroupHeader(parent)
{
}

KFileItemListGroupHeader::~KFileItemListGroupHeader()
{
}

void KFileItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    KItemListGroupHeader::paint(painter, option, widget);
    // TODO: Use dataChanged() hook to prepare a cached property
    painter->drawText(QRectF(0, 0, size().width(), size().height()), data().toString());
}

#include "kfileitemlistgroupheader.moc"
