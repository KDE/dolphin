/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz@gmx.at>             *
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

#ifndef DOLPHINICONSVIEW_H
#define DOLPHINICONSVIEW_H

#include <kcategorizedview.h>

#include <kfileitem.h>
#include <kfileitemdelegate.h>

#include <QFont>
#include <QSize>
#include <QStyleOption>

#include <libdolphin_export.h>

class DolphinController;
class SelectionManager;
class DolphinCategoryDrawer;
class DolphinSortFilterProxyModel;
class DolphinViewAutoScroller;

/**
 * @brief Represents the view, where each item is shown as an icon.
 *
 * It is also possible that instead of the icon a preview of the item
 * content is shown.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinIconsView : public KCategorizedView
{
    Q_OBJECT

public:
    explicit DolphinIconsView(QWidget* parent,
                              DolphinController* controller,
                              DolphinSortFilterProxyModel* proxyModel);
    virtual ~DolphinIconsView();

protected slots:
    virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void slotShowPreviewChanged();
    void slotAdditionalInfoChanged();
    void setNameFilter(const QString& nameFilter);
    void setZoomLevel(int level);
    void requestActivation();
    void slotGlobalSettingsChanged(int category);

private:
    /**
     * Updates the size of the grid depending on the state
     * of \a showPreview and \a additionalInfoCount.
     */
    void updateGridSize(bool showPreview, int additionalInfoCount);

    /**
     * Returns the number of additional information lines that should
     * be shown below the item name.
     */
    int additionalInfoCount() const;

private:
    DolphinController* m_controller;
    SelectionManager* m_selectionManager;
    DolphinViewAutoScroller* m_autoScroller;
    DolphinCategoryDrawer* m_categoryDrawer;

    QFont m_font;
    QSize m_decorationSize;
    QStyleOptionViewItem::Position m_decorationPosition;
    Qt::Alignment m_displayAlignment;

    QSize m_itemSize;
    QRect m_dropRect;
};

#endif
