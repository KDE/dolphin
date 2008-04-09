/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#ifndef TOOLTIPMANAGER_H
#define TOOLTIPMANAGER_H

#include <QObject>
#include <QPoint>

#include <kfileitem.h>
#include <kformattedballoontipdelegate.h>

class DolphinModel;
class DolphinSortFilterProxyModel;
class QAbstractItemView;
class QModelIndex;
class QTimer;

/**
 * @brief Manages the tooltips for an item view.
 *
 * When hovering an item, a tooltip is shown after
 * a short timeout. The tooltip is hidden again when the
 * viewport is hovered or the item view has been left.
 */
class ToolTipManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolTipManager(QAbstractItemView* parent,
                            DolphinSortFilterProxyModel* model);
    virtual ~ToolTipManager();

    /**
     * Hides the currently shown tooltip. Invoking this method is
     * only needed when the tooltip should be hidden although
     * an item is hovered.
     */
    void hideTip();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void requestToolTip(const QModelIndex& index);
    void hideToolTip();
    void showToolTip();

private:
    QAbstractItemView* m_view;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;

    QTimer* m_timer;
    KFileItem m_item;
    QPoint m_pos;
    KFormattedBalloonTipDelegate m_delegate;
};

#endif
