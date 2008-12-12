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
 * Extends KFileItemDelegate by the ability to show the hover effect
 * and the selection in a minimized way for the name column of
 * the details view.
 *
 * Note that this is a workaround, as Qt does not support having custom
 * shapes within the visual rect of an item view. The visual part of
 * workaround is handled inside DolphinFileItemDelegate, the behavior
 * changes are handled in DolphinDetailsView.
 */
class DolphinFileItemDelegate : public KFileItemDelegate
{
public:
    explicit DolphinFileItemDelegate(QObject* parent = 0);
    virtual ~DolphinFileItemDelegate();

    /**
     * If \a minimized is true, the hover effect and the selection are
     * only drawn above the icon and text of an item. Per default
     * \a minimized is false, which means that the whole visual rect is
     * used like in KFileItemDelegate.
     */
    void setMinimizedNameColumn(bool minimized);
    bool hasMinimizedNameColumn() const;

    virtual void paint(QPainter* painter,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;

    /**
     * Returns the minimized width of the name column for the name \a name. This method
     * is also used in DolphinDetailsView to handle the selection of items correctly.
     */
    static int nameColumnWidth(const QString& name, const QStyleOptionViewItem& option);

private:
    bool m_hasMinimizedNameColumn;
};

inline void DolphinFileItemDelegate::setMinimizedNameColumn(bool minimized)
{
    m_hasMinimizedNameColumn = minimized;
}

inline bool DolphinFileItemDelegate::hasMinimizedNameColumn() const
{
    return m_hasMinimizedNameColumn;
}

#endif
