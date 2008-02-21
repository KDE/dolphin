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
class DolphinCategoryDrawer;

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
    explicit DolphinIconsView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinIconsView();

    /** @see QAbstractItemView::visualRect() */
    virtual QRect visualRect(const QModelIndex& index) const;

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    void triggerItem(const QModelIndex& index);
    void slotEntered(const QModelIndex& index);
    void slotShowPreviewChanged();
    void slotAdditionalInfoChanged();
    void zoomIn();
    void zoomOut();
    void requestActivation();
    void updateFont();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    /** Returns the increased icon size for the size \a size. */
    int increasedIconSize(int size) const;

    /** Returns the decreased icon size for the size \a size. */
    int decreasedIconSize(int size) const;

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
    DolphinCategoryDrawer* m_categoryDrawer;

    QFont m_font;
    QSize m_decorationSize;
    QStyleOptionViewItem::Position m_decorationPosition;
    Qt::Alignment m_displayAlignment;

    QSize m_itemSize;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
};

#endif
