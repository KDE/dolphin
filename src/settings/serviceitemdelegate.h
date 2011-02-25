/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef SERVICEITEMDELEGATE_H
#define SERVICEITEMDELEGATE_H

#include <KWidgetItemDelegate>

/**
 * @brief Widget item delegate for a service that can be enabled or disabled.
 *
 * Additionally it is possible to configure a service.
 * @see ServiceModel
 */
class ServiceItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    explicit ServiceItemDelegate(QAbstractItemView* itemView, QObject* parent = 0);
    virtual ~ServiceItemDelegate();

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;

    virtual QList<QWidget*> createItemWidgets() const;

    virtual void updateItemWidgets(const QList<QWidget*> widgets,
                                   const QStyleOptionViewItem& option,
                                   const QPersistentModelIndex& index) const;

signals:
    void requestServiceConfiguration(const QModelIndex& index);

private slots:
    void slotCheckBoxClicked(bool checked);
    void slotConfigureButtonClicked();
};

#endif
