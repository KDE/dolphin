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

#ifndef DOLPHINFILEITEMDELEGATE_H
#define DOLPHINFILEITEMDELEGATE_H

#include <kfileitemdelegate.h>

/**
 * @brief Extends KFileItemDelegate with the ability to set
 *        a maximum size.
 */
class DolphinFileItemDelegate : public KFileItemDelegate
{
    Q_OBJECT

public:
    explicit DolphinFileItemDelegate(QObject* parent = 0);
    virtual ~DolphinFileItemDelegate();

    void setMaximumSize(const QSize& size);
    QSize maximumSize() const;

    /** @see QItemDelegate::sizeHint() */
    virtual QSize sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const;

private:
    QSize m_maxSize;
};

#endif
