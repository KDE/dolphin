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

#include "dolphinview.h"

#include <kurl.h>

#include <QAbstractItemView>
#include <QList>
#include <QString>
#include <QStyleOption>

class DolphinColumnWidget;
class DolphinController;
class DolphinModel;
class QAbstractProxyModel;
class QFrame;
class QTimeLine;

/**
 * @brief Represents the view, where each directory is show as separate column.
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 */
class DolphinColumnView : public QAbstractItemView
{
    Q_OBJECT

public:
    explicit DolphinColumnView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinColumnView();

    /** @see QAbstractItemView::indexAt() */
    virtual QModelIndex indexAt(const QPoint& point) const;

    /**
     * Returns the item on the position \a pos. The KFileItem instance
     * is null if no item is below the position.
     */
    KFileItem itemAt(const QPoint& point) const;

    /** @see QAbstractItemView::scrollTo() */
    virtual void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);

    /** @see QAbstractItemView::visualRect() */
    virtual QRect visualRect(const QModelIndex& index) const;

    /** Inverts the selection of the currently active column. */
    void invertSelection();

    /**
     * Reloads the content of all columns. In opposite to non-hierarchical views
     * it is not enough to reload the KDirLister, instead this method must be explicitly
     * invoked.
     */
    void reload();

    /**
     * Adjusts the root URL of the first column and removes all
     * other columns.
     */
    void setRootUrl(const KUrl& url);

    /** Returns the URL of the first column. */
    KUrl rootUrl() const;

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);

    /**
     * Returns the currently used name filter. All items
     * which contain the name filter will be shown.
     */
    QString nameFilter() const;

    /**
     * Shows the column which represents the URL \a url. If the column
     * is already shown, it gets activated, otherwise it will be created.
     */
    void showColumn(const KUrl& url);

    /**
     * Does an inline editing for the item \a item
     * inside the active column.
     */
    void editItem(const KFileItem& item);

    /**
     * Returns the selected items of the active column.
     */
    KFileItemList selectedItems() const;

public slots:
    /** @see QAbstractItemView::selectAll() */
    virtual void selectAll();

protected:
    virtual bool isIndexHidden(const QModelIndex& index) const;
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    virtual void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags);
    virtual QRegion visualRegionForSelection(const QItemSelection& selection) const;
    virtual int horizontalOffset() const;
    virtual int verticalOffset() const;

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    void zoomIn();
    void zoomOut();

    /**
     * Moves the content of the columns view to represent
     * the scrollbar position \a x.
     */
    void moveContentHorizontally(int x);

    /**
     * Updates the size of the decoration dependent on the
     * icon size of the ColumnModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize();

    /**
     * Updates the background color of the columns to respect
     * the current activation state \a active.
     */
    void updateColumnsBackground(bool active);

    void slotSortingChanged(DolphinView::Sorting sorting);
    void slotSortOrderChanged(Qt::SortOrder order);
    void slotShowHiddenFilesChanged();
    void slotShowPreviewChanged();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    DolphinColumnWidget* activeColumn() const;

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
    void requestActivation(DolphinColumnWidget* column);

    /** Removes all columns except of the root column. */
    void removeAllColumns();

    /**
     * Returns the position of the point \a point relative to the column
     * \a column.
     */
    QPoint columnPosition(DolphinColumnWidget* column, const QPoint& point) const;

private:
    DolphinController* m_controller;
    bool m_active;
    int m_index;
    int m_contentX;
    QList<DolphinColumnWidget*> m_columns;
    QFrame* m_emptyViewport;
    QTimeLine* m_animation;
    QString m_nameFilter;

    friend class DolphinColumnWidget;
};

inline DolphinColumnWidget* DolphinColumnView::activeColumn() const
{
    return m_columns[m_index];
}

#endif
