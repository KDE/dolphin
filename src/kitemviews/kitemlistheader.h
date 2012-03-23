/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KITEMLISTHEADER_H
#define KITEMLISTHEADER_H

#include <libdolphin_export.h>
#include <QHash>
#include <QObject>

class KItemListHeaderWidget;
class KItemListView;

/**
 * @brief Provides access to the header of a KItemListView.
 *
 * Each column of the header represents a visible role
 * accessible by KItemListView::visibleRoles().
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListHeader : public QObject
{
    Q_OBJECT

public:
    virtual ~KItemListHeader();

    /**
     * If set to true, KItemListView will automatically adjust the
     * widths of the columns. If set to false, the size can be
     * manually adjusted by KItemListHeader::setColumnWidth().
     */
    void setAutomaticColumnResizing(bool automatic);
    bool automaticColumnResizing() const;

    /**
     * Sets the width of the column for the given role. Note that
     * the width only gets applied if KItemListHeader::automaticColumnResizing()
     * has been turned off.
     */
    void setColumnWidth(const QByteArray& role, qreal width);
    qreal columnWidth(const QByteArray& role) const;

    /**
     * Sets the widths of the columns for all roles. From a performance point of
     * view calling this method should be preferred over several setColumnWidth()
     * calls in case if the width of more than one column should be changed.
     * Note that the widths only get applied if KItemListHeader::automaticColumnResizing()
     * has been turned off.
     */
    void setColumnWidths(const QHash<QByteArray, qreal>& columnWidths);

signals:
    /**
     * Is emitted if the width of a column has been adjusted by the user with the mouse
     * (no signal is emitted if KItemListHeader::setColumnWidth() is invoked).
     */
    void columnWidthChanged(const QByteArray& role,
                            qreal currentWidth,
                            qreal previousWidth);

private:
    KItemListHeader(KItemListView* listView);

private:
    KItemListView* m_view;
    KItemListHeaderWidget* m_headerWidget;

    friend class KItemListView; // Constructs the KItemListHeader instance
};

#endif


