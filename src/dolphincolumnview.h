/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#include <QColumnView>
#include <QStyleOption>

class DolphinController;
class KDirLister;
class KUrl;

/**
 * @brief Represents the view, where each directory is show as separate column.
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 */
class DolphinColumnView : public QColumnView
{
    Q_OBJECT

public:
    explicit DolphinColumnView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinColumnView();

protected:
    virtual QAbstractItemView* createColumn(const QModelIndex& index);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);

private slots:
    void zoomIn();
    void zoomOut();
    void triggerItem(const QModelIndex& index);

    /**
     * Updates the activation state of all columns, where \a url
     * represents the URL of the active column. All operations
     * are applied only to the column which is marked as active.
     */
    void updateColumnsState(const KUrl& url);

    /**
     * Updates the size of the decoration dependent on the
     * icon size of the ColumnModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    /**
     * Requests the activation for the column \a column. The URL
     * navigator will be changed to represent the column. It is
     * assured that the selection model of \a column will be set
     * to the selection model of the Column View.
     */
    void requestActivation(QWidget* column);

    /**
     * Requests the selection model from the Column View for \a view.
     * If another column has already obtained the Column View selection
     * model, it will be replaced by a default selection model.
     */
    void requestSelectionModel(QAbstractItemView* view);

private:
    DolphinController* m_controller;

    friend class ColumnWidget;
};

#endif
