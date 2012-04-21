/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#ifndef KITEMLISTCONTAINER_H
#define KITEMLISTCONTAINER_H

#include <libdolphin_export.h>

#include <QAbstractAnimation>
#include <QAbstractScrollArea>

class KItemListController;
class KItemListSmoothScroller;
class KItemListView;
class KItemModelBase;
class QPropertyAnimation;

/**
 * @brief Provides a QWidget based scrolling view for a KItemListController.
 *
 * The model and view are maintained by the KItemListController.
 *
 * @see KItemListController
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListContainer : public QAbstractScrollArea
{
    Q_OBJECT

public:
    /**
     * @param controller Controller that maintains the model and the view.
     *                   The KItemListContainer takes ownership of the controller
     *                   (the parent will be set to the KItemListContainer).
     * @param parent     Optional parent widget.
     */
    explicit KItemListContainer(KItemListController* controller, QWidget* parent = 0);
    virtual ~KItemListContainer();
    KItemListController* controller() const;

    void setEnabledFrame(bool enable);
    bool enabledFrame() const;

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void scrollContentsBy(int dx, int dy);
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    void slotScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    void slotModelChanged(KItemModelBase* current, KItemModelBase* previous);
    void slotViewChanged(KItemListView* current, KItemListView* previous);
    void scrollTo(qreal offset);
    void updateScrollOffsetScrollBar();
    void updateItemOffsetScrollBar();

private:
    void updateGeometries();
    void updateSmoothScrollers(Qt::Orientation orientation);

    /**
     * Helper method for updateScrollOffsetScrollBar(). Updates the scrollbar-policy
     * to Qt::ScrollBarAlwaysOn for cases where turning off the scrollbar might lead
     * to an endless layout loop (see bug #293318).
     */
    void updateScrollOffsetScrollBarPolicy();

private:
    KItemListController* m_controller;

    KItemListSmoothScroller* m_horizontalSmoothScroller;
    KItemListSmoothScroller* m_verticalSmoothScroller;
};

#endif


