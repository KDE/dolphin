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

#include <klistview.h>
#include <kitemcategorizer.h>
#include <QtGui/QStyleOption>
#include <libdolphin_export.h>

class DolphinController;

/**
 * @brief Represents the view, where each item is shown as an icon.
 *
 * It is also possible that instead of the icon a preview of the item
 * content is shown.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinIconsView : public KListView
{
    Q_OBJECT

public:
    explicit DolphinIconsView(QWidget* parent, DolphinController* controller);
    virtual ~DolphinIconsView();

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);

private slots:
    void slotShowPreviewChanged(bool show);
    void slotShowAdditionalInfoChanged(bool show);
    void zoomIn();
    void zoomOut();

private:
    bool isZoomInPossible() const;
    bool isZoomOutPossible() const;

    /** Returns the increased icon size for the size \a size. */
    int increasedIconSize(int size) const;

    /** Returns the decreased icon size for the size \a size. */
    int decreasedIconSize(int size) const;

    /**
     * Updates the size of the grid depending on the state
     * of \a showPreview and \a showAdditionalInfo.
     */
    void updateGridSize(bool showPreview, bool showAdditionalInfo);

private:
    DolphinController* m_controller;
    QStyleOptionViewItem m_viewOptions;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
};

#endif
