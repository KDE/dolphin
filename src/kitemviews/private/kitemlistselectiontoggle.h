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

#ifndef KITEMLISTSELECTIONTOGGLE_H
#define KITEMLISTSELECTIONTOGGLE_H

#include "dolphin_export.h"

#include <QGraphicsWidget>
#include <QPixmap>


/**
 * @brief Allows to toggle between the selected and unselected state of an item.
 */
class DOLPHIN_EXPORT KItemListSelectionToggle : public QGraphicsWidget
{
    Q_OBJECT

public:
    KItemListSelectionToggle(QGraphicsItem* parent);
    ~KItemListSelectionToggle() override;

    void setChecked(bool checked);
    bool isChecked() const;

    void setHovered(bool hovered);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

private:
    void updatePixmap();
    int iconSize() const;

private:
    bool m_checked;
    bool m_hovered;
    QPixmap m_pixmap;
};

#endif


