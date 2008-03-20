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

#include "dolphinfileitemdelegate.h"

DolphinFileItemDelegate::DolphinFileItemDelegate(QObject* parent) :
    KFileItemDelegate(parent),
    m_maxSize(0, 0)
{
}

DolphinFileItemDelegate::~DolphinFileItemDelegate()
{
}

void DolphinFileItemDelegate::setMaximumSize(const QSize& size)
{
    m_maxSize = size;
}


QSize DolphinFileItemDelegate::maximumSize() const
{
    return m_maxSize;
}

QSize DolphinFileItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    QSize size = KFileItemDelegate::sizeHint(option, index);

    const int maxWidth = m_maxSize.width();
    if ((maxWidth > 0) && (size.width() > maxWidth)) {
        size.setWidth(maxWidth);
    }

    const int maxHeight = m_maxSize.height();
    if ((maxHeight > 0) && (size.height() > maxHeight)) {
        size.setHeight(maxHeight);
    }

    return size;
}

#include "dolphinfileitemdelegate.moc"
