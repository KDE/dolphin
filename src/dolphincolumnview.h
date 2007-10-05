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

#include <QAbstractItemView>
#include <QList>
#include <QStyleOption>

class DolphinColumnWidget;
class DolphinController;
class DolphinModel;
class KUrl;
class QAbstractProxyModel;
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

    virtual QModelIndex indexAt(const QPoint& point) const;
    virtual void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);
    virtual QRect visualRect(const QModelIndex& index) const;
    virtual void setModel(QAbstractItemModel* model);

    /** Inverts the selection of the currently active column. */
    void invertSelection();

    /**
     * Reloads the content of all columns. In opposite to non-hierarchical views
     * it is not enough to reload the KDirLister, instead this method must be explicitly
     * invoked.
     */
    void reload();

public slots:
    /**
     * Shows the column which represents the URL \a url. If the column
     * is already shown, it gets activated, otherwise it will be created.
     */
    void showColumn(const KUrl& url);

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

    void slotShowHiddenFilesChanged(bool show);
    void slotShowPreviewChanged(bool show);

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

private:
    DolphinController* m_controller;
    bool m_restoreActiveColumnFocus;
    int m_index;
    int m_contentX;
    QList<DolphinColumnWidget*> m_columns;
    QTimeLine* m_animation;

    DolphinModel* m_dolphinModel;
    QAbstractProxyModel* m_proxyModel;

    friend class DolphinColumnWidget;
};

inline DolphinColumnWidget* DolphinColumnView::activeColumn() const
{
    return m_columns[m_index];
}

#endif
