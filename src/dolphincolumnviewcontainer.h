/***************************************************************************
 *   Copyright (C) 2007-2009 by Peter Penz <peter.penz@gmx.at>             *
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

#ifndef DOLPHINCOLUMNVIEWCONTAINER_H
#define DOLPHINCOLUMNVIEWCONTAINER_H

#include "dolphinview.h"

#include <kurl.h>

#include <QList>
#include <QScrollArea>
#include <QString>

class DolphinColumnView;
class DolphinController;
class QFrame;
class QTimeLine;

/**
 * @brief Represents a container for columns represented as instances
 *        of DolphinColumnView.
 *
 * @see DolphinColumnView
 */
class DolphinColumnViewContainer : public QScrollArea
{
    Q_OBJECT

public:
    explicit DolphinColumnViewContainer(QWidget* parent,
                                        DolphinController* controller);
    virtual ~DolphinColumnViewContainer();

    KUrl rootUrl() const;

    QAbstractItemView* activeColumn() const;

    /**
     * Shows the column which represents the URL \a url. If the column
     * is already shown, it gets activated, otherwise it will be created.
     */
    bool showColumn(const KUrl& url);

signals:
    /**
     * Requests that the given column be deleted at the discretion
     * of the receiver of the signal.
     */
    void requestColumnDeletion(QAbstractItemView* column);

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    /**
     * Moves the content of the columns view to represent
     * the scrollbar position \a x.
     */
    void moveContentHorizontally(int x);

    /**
     * Updates the background color of the columns to respect
     * the current activation state \a active.
     */
    void updateColumnsBackground(bool active);

private:
    /**
     * Deactivates the currently active column and activates
     * the new column indicated by \a index. m_index represents
     * the active column afterwards. Also the URL of the navigator
     * will be adjusted to reflect the column URL.
     */
    void setActiveColumnIndex(int index);

    void layoutColumns();
    void updateScrollBar();

    /**
     * Assures that the currently active column is fully visible
     * by adjusting the horizontal position of the content.
     */
    void assureVisibleActiveColumn();

    /**
     * Request the activation for the column \a column. It is assured
     * that the columns gets fully visible by adjusting the horizontal
     * position of the content.
     */
    void requestActivation(DolphinColumnView* column);

    /** Removes all columns except of the root column. */
    void removeAllColumns();

    /**
     * Returns the position of the point \a point relative to the column
     * \a column.
     */
    QPoint columnPosition(DolphinColumnView* column, const QPoint& point) const;

    /**
     * Deletes the column. If the itemview of the controller is set to the column,
     * the controllers itemview is set to 0.
     */
    void deleteColumn(DolphinColumnView* column);

private:
    DolphinController* m_controller;
    bool m_active;
    int m_index;
    int m_contentX;
    QList<DolphinColumnView*> m_columns;
    QFrame* m_emptyViewport;
    QTimeLine* m_animation;

    friend class DolphinColumnView;
};

#endif
