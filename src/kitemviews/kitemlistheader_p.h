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

#ifndef KITEMLISTHEADER_H
#define KITEMLISTHEADER_H

#include <libdolphin_export.h>
#include <QGraphicsWidget>
#include <QHash>
#include <QList>

class KItemModelBase;

/**
 * @brief Header for KItemListView that shows the currently used roles.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListHeader : public QGraphicsWidget
{
    Q_OBJECT

public:
    KItemListHeader(QGraphicsWidget* parent = 0);
    virtual ~KItemListHeader();

    void setModel(KItemModelBase* model);
    KItemModelBase* model() const;

    void setVisibleRoles(const QList<QByteArray>& roles);
    QList<QByteArray> visibleRoles() const;

    void setVisibleRolesWidths(const QHash<QByteArray, qreal> rolesWidths);
    QHash<QByteArray, qreal> visibleRolesWidths() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

private slots:
    void slotSortRoleChanged(const QByteArray& current, const QByteArray& previous);
    void slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

private:
    KItemModelBase* m_model;
    QList<QByteArray> m_visibleRoles;
    QHash<QByteArray, qreal> m_visibleRolesWidths;
};

#endif


