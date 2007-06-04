/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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

#ifndef DOLPHINCOLUMNVIEW_H
#define DOLPHINCOLUMNVIEW_H

#include <QtGui/QColumnView>
#include <QtGui/QStyleOption>

class DolphinController;

/**
 * @brief TODO
 */
class DolphinColumnView : public QColumnView
{
    Q_OBJECT

public:
    explicit DolphinColumnView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinColumnView();

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);

private slots:
    void zoomIn();
    void zoomOut();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    /**
     * Updates the size of the decoration dependent on the
     * icon size of the ColumnModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize();

private:
    DolphinController* m_controller;
    QStyleOptionViewItem m_viewOptions;
};

#endif
