/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#ifndef DOLPHINDETAILSVIEW_H
#define DOLPHINDETAILSVIEW_H

#include "dolphintreeview.h"
#include <QTreeView>
#include <libdolphin_export.h>
#include <views/dolphinview.h>

class DolphinViewController;
class DolphinSortFilterProxyModel;
class ViewExtensionsFactory;

/**
 * @brief Represents the details view which shows the name, size,
 *        date, permissions, owner and group of an item.
 *
 * The width of the columns is automatically adjusted in a way
 * that full available width of the view is used by stretching the width
 * of the name column.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinDetailsView : public DolphinTreeView
{
    Q_OBJECT

public:
    /**
     * @param parent                Parent widget.
     * @param dolphinViewController Allows the DolphinDetailsView to control the
     *                              DolphinView in a limited way.
     * @param viewModeController    Controller that is used by the DolphinView
     *                              to control the DolphinDetailsView. The DolphinDetailsView
     *                              only has read access to the controller.
     * @param model                 Directory that is shown.
     */
    explicit DolphinDetailsView(QWidget* parent,
                                DolphinViewController* dolphinViewController,
                                const ViewModeController* viewModeController,
                                DolphinSortFilterProxyModel* model);
    virtual ~DolphinDetailsView();

    /**
     * Returns a set containing the URLs of all expanded items.
     */
    QSet<KUrl> expandedUrls() const;

public:
    virtual QRect visualRect(const QModelIndex& index) const;

protected:
    virtual bool event(QEvent* event);
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    virtual bool eventFilter(QObject* watched, QEvent* event);
    virtual bool acceptsDrop(const QModelIndex& index) const;

protected slots:
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private slots:
    /**
     * Sets the sort indicator section of the header view
     * corresponding to \a sorting.
     */
    void setSortIndicatorSection(DolphinView::Sorting sorting);

    /**
     * Sets the sort indicator order of the header view
     * corresponding to \a sortOrder.
     */
    void setSortIndicatorOrder(Qt::SortOrder sortOrder);

    /**
     * Synchronizes the sorting state of the Dolphin menu 'View -> Sort'
     * with the current state of the details view.
     * @param column Index of the current sorting column.
     */
    void synchronizeSortingState(int column);

    /**
     * Is invoked when the mouse cursor has entered an item. The controller
     * gets informed to emit the itemEntered() signal if the mouse cursor
     * is above the name column. Otherwise the controller gets informed
     * to emit the itemViewportEntered() signal (all other columns should
     * behave as viewport area).
     */
    void slotEntered(const QModelIndex& index);

    void setZoomLevel(int level);

    void slotShowPreviewChanged();

    /**
     * Opens a context menu at the position \a pos and allows to
     * configure the visibility of the header columns and whether
     * expandable folders should be shown.
     */
    void configureSettings(const QPoint& pos);

    /**
     * Updates the visibilty state of columns and their order.
     */
    void updateColumnVisibility();

    /**
     * Resizes all columns in a way to use the whole available width of the view.
     */
    void resizeColumns();

    /**
     * Saves order of the columns as global setting.
     */
    void saveColumnPositions();

    /**
     * Disables the automatical resizing of columns, if the user has resized the columns
     * with the mouse.
     */
    void slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize);

    /**
     * Changes the alternating row colors setting depending from
     * the activation state \a active.
     */
    void slotActivationChanged(bool active);

    /**
     * Disables the automatical resizing of the columns. Per default all columns
     * are resized to use the maximum available width of the view as good as possible.
     */
    void disableAutoResizing();

    void requestActivation();

    void slotGlobalSettingsChanged(int category);

    /**
     * If \a expandable is true, the details view acts as tree view.
     * The current expandable state is remembered in the settings.
     */
    void setFoldersExpandable(bool expandable);

    /**
     * These slots update the list of expanded items.
     */
    void slotExpanded(const QModelIndex& index);
    void slotCollapsed(const QModelIndex& index);

private:
    /**
     * Removes the URLs corresponding to the children of \a index in the rows
     * between \a start and \a end inclusive from the set of expanded URLs.
     */
    void removeExpandedIndexes(const QModelIndex& parent, int start, int end);

    /**
     * Updates the size of the decoration dependent on the
     * icon size of the DetailsModeSettings. The controller
     * will get informed about possible zoom in/zoom out
     * operations.
     */
    void updateDecorationSize(bool showPreview);

    KFileItemDelegate::Information infoForColumn(int columnIndex) const;

    /**
     * Sets the maximum size available for editing in the delegate.
     */
    void adjustMaximumSizeForEditing(const QModelIndex& index);

private:
    bool m_autoResize; // if true, the columns are resized automatically to the available width

    DolphinViewController* m_dolphinViewController;
    const ViewModeController* m_viewModeController;
    ViewExtensionsFactory* m_extensionsFactory;
    QAction* m_expandableFoldersAction;

    // A set containing the URLs of all currently expanded folders.
    // We cannot use a QSet<QModelIndex> because a QModelIndex is not guaranteed to remain valid over time.
    // Also a QSet<QPersistentModelIndex> does not work as expected because it is not guaranteed that
    // subsequent expand/collapse events of the same file item will yield the same QPersistentModelIndex.
    QSet<KUrl> m_expandedUrls;

    QFont m_font;
    QSize m_decorationSize;
};

#endif
